#include "Server.hpp"
#include <Geode/utils/JsonValidation.hpp>
#include <Geode/utils/ranges.hpp>
#include <chrono>
#include <date/date.h>
#include <fmt/core.h>
#include <loader/ModMetadataImpl.hpp>
#include <fmt/chrono.h>
#include <loader/LoaderImpl.hpp>
#include "../internal/about.hpp"
#include "Geode/loader/Loader.hpp"

using namespace server;

#define GEODE_GD_VERSION_STR GEODE_STR(GEODE_GD_VERSION)

template <class K, class V>
    requires std::equality_comparable<K> && std::copy_constructible<K>
class CacheMap final {
private:
    // I know this looks like a goofy choice over just
    // `std::unordered_map`, but hear me out:
    //
    // This needs preserved insertion order (so shrinking the cache
    // to match size limits doesn't just have to erase random
    // elements)
    //
    // If this used a map for values and another vector for storing
    // insertion order, it would have a pretty big memory footprint
    // (two copies of Query, one for order, one for map + two heap
    // allocations on top of that)
    //
    // In addition, it would be a bad idea to have a cache of 1000s
    // of items in any case (since that would likely take up a ton
    // of memory, which we want to avoid since it's likely many
    // crashes with the old index were due to too much memory
    // usage)
    //
    // Linear searching a vector of at most a couple dozen items is
    // lightning-fast (🚀), and besides the main performance benefit
    // comes from the lack of a web request - not how many extra
    // milliseconds we can squeeze out of a map access
    std::vector<std::pair<K, V>> m_values;
    size_t m_sizeLimit = 20;

public:
    std::optional<V> get(K const& key) {
        auto it = std::find_if(m_values.begin(), m_values.end(), [key](auto const& q) {
            return q.first == key;
        });
        if (it != m_values.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    void add(K&& key, V&& value) {
        auto pair = std::make_pair(std::move(key), std::move(value));

        // Shift and replace last element if we're at cache size limit
        if (m_values.size() >= m_sizeLimit) {
            std::shift_left(m_values.begin(), m_values.end(), 1);
            m_values.back() = std::move(pair);
        }
        // Otherwise append at end
        else {
            m_values.emplace_back(std::move(pair));
        }
    }
    void remove(K const& key) {
        ranges::remove(m_values, [&key](auto const& q) { return q.first == key; });
    }
    void clear() {
        m_values.clear();
    }
    void limit(size_t size) {
        m_sizeLimit = size;
        m_values.clear();
    }
    size_t size() const {
        return m_values.size();
    }
    size_t limit() const {
        return m_sizeLimit;
    }
};

template <class F>
struct ExtractFun;

template <class V, class... Args>
struct ExtractFun<ServerRequest<V>(*)(Args...)> {
    using CacheKey = std::tuple<std::remove_cvref_t<Args>...>;
    using Value = V;

    template <class... CArgs>
    static CacheKey key(CArgs const&... args) {
        return std::make_tuple(args..., false);
    }
    template <class... CArgs>
    static ServerRequest<V> invoke(auto&& func, CArgs const&... args) {
        return func(args..., false);
    }
};

template <auto F>
class FunCache final {
public:
    using Extract  = ExtractFun<decltype(F)>;
    using CacheKey = typename Extract::CacheKey;
    using Value    = typename Extract::Value;

private:
    std::mutex m_mutex;
    CacheMap<CacheKey, ServerRequest<Value>> m_cache;

public:
    FunCache() = default;
    FunCache(FunCache const&) = delete;
    FunCache(FunCache&&) = delete;

    template <class... Args>
    ServerRequest<Value> get(Args const&... args) {
        std::unique_lock lock(m_mutex);
        if (auto v = m_cache.get(Extract::key(args...))) {
            return *v;
        }
        auto f = Extract::invoke(F, args...);
        m_cache.add(Extract::key(args...), ServerRequest<Value>(f));
        return f;
    }

    template <class... Args>
    void remove(Args const&... args) {
        std::unique_lock lock(m_mutex);
        m_cache.remove(Extract::key(args...));
    }

    size_t size() {
        std::unique_lock lock(m_mutex);
        return m_cache.size();
    }
    void limit(size_t size) {
        std::unique_lock lock(m_mutex);
        m_cache.limit(size);
    }
    void clear() {
        std::unique_lock lock(m_mutex);
        m_cache.clear();
    }
};

template <auto F>
FunCache<F>& getCache() {
    static auto inst = FunCache<F>();
    return inst;
}

static const char* jsonTypeToString(matjson::Type const& type) {
    switch (type) {
        case matjson::Type::Object: return "object";
        case matjson::Type::Array: return "array";
        case matjson::Type::Bool: return "boolean";
        case matjson::Type::Number: return "number";
        case matjson::Type::String: return "string";
        case matjson::Type::Null: return "null";
        default: return "unknown";
    }
}

static Result<matjson::Value, ServerError> parseServerPayload(web::WebResponse const& response) {
    auto asJson = response.json();
    if (!asJson) {
        return Err(ServerError(response.code(), "Response was not valid JSON: {}", asJson.unwrapErr()));
    }
    auto json = std::move(asJson).unwrap();
    if (!json.isObject()) {
        return Err(ServerError(response.code(), "Expected object, got {}", jsonTypeToString(json.type())));
    }
    if (!json.contains("payload")) {
        return Err(ServerError(response.code(), "Object does not contain \"payload\" key - got {}", json.dump()));
    }
    return Ok(json["payload"]);
}

static ServerError parseServerError(web::WebResponse const& error) {
    // The server should return errors as `{ "error": "...", "payload": "" }`
    if (auto asJson = error.json()) {
        auto json = asJson.unwrap();
        if (json.isObject() && json.contains("error") && json["error"].isString()) {
            return ServerError(
                error.code(),
                "{}", json["error"].asString().unwrapOr("Unknown (no error message)")
            );
        }
        else {
            return ServerError(error.code(), "Unknown (not valid JSON)");
        }
    }
    // But if we get something else for some reason, return that
    else {
        return ServerError(
            error.code(),
            "{}", error.string().unwrapOr("Unknown (not a valid string)")
        );
    }
}

static ServerProgress parseServerProgress(web::WebProgress const& prog, auto msg) {
    if (auto per = prog.downloadProgress()) {
        return ServerProgress(msg, static_cast<uint8_t>(*per));
    }
    else {
        return ServerProgress(msg);
    }
}

const char* server::sortToString(ModsSort sorting) {
    switch (sorting) {
        default:
        case ModsSort::Downloads: return "downloads";
        case ModsSort::RecentlyUpdated: return "recently_updated";
        case ModsSort::RecentlyPublished: return "recently_published";
    }
}

std::string ServerDateTime::toAgoString() const {
    auto const fmtPlural = [](auto count, auto unit) {
        if (count == 1) {
            return fmt::format("{} {} ago", count, unit);
        }
        return fmt::format("{} {}s ago", count, unit);
    };
    auto now = Clock::now();
    auto len = std::chrono::duration_cast<std::chrono::minutes>(now - value).count();
    if (len < 60) {
        return fmtPlural(len, "minute");
    }
    len = std::chrono::duration_cast<std::chrono::hours>(now - value).count();
    if (len < 24) {
        return fmtPlural(len, "hour");
    }
    len = std::chrono::duration_cast<std::chrono::days>(now - value).count();
    if (len < 31) {
        return fmtPlural(len, "day");
    }
    return fmt::format("{:%b %d %Y}", value);
}

Result<ServerTag> ServerTag::parse(matjson::Value const& raw) {
    auto root = checkJson(raw, "ServerTag");
    auto res = ServerTag();

    root.needs("id").into(res.id);
    root.needs("name").into(res.name);
    root.needs("display_name").into(res.displayName);

    return root.ok(res);
}
Result<std::vector<ServerTag>> ServerTag::parseList(matjson::Value const& raw) {
    auto payload = checkJson(raw, "ServerTagsList");
    std::vector<ServerTag> list {};
    for (auto& item : payload.items()) {
        auto mod = ServerTag::parse(item.json());
        if (mod) {
            list.push_back(mod.unwrap());
        }
        else {
            log::error("Unable to parse tag from the server: {}", mod.unwrapErr());
        }
    }
    return payload.ok(list);
}

Result<ServerDateTime> ServerDateTime::parse(std::string const& str) {
    std::stringstream ss(str);
    date::sys_seconds seconds;
    if (ss >> date::parse("%Y-%m-%dT%H:%M:%S%Z", seconds)) {
        return Ok(ServerDateTime {
            .value = seconds
        });
    }
    return Err("Invalid date time format '{}'", str);
}

Result<ServerModVersion> ServerModVersion::parse(matjson::Value const& raw) {
    auto root = checkJson(raw, "ServerModVersion");

    auto res = ServerModVersion();

    res.metadata.setGeodeVersion(root.needs("geode").get<VersionInfo>());

    // Verify target GD version
    auto gd_obj = root.needs("gd");
    std::string gd = "0.000";
    if (gd_obj.hasNullable(GEODE_PLATFORM_SHORT_IDENTIFIER)) {
        gd = gd_obj.hasNullable(GEODE_PLATFORM_SHORT_IDENTIFIER). get<std::string>();
    }

    if (gd != "*") {
        res.metadata.setGameVersion(gd);
    }

    // Get server info
    root.needs("download_link").into(res.downloadURL);
    root.needs("download_count").into(res.downloadCount);
    root.needs("hash").into(res.hash);

    // Get mod metadata info
    res.metadata.setID(root.needs("mod_id").get<std::string>());
    res.metadata.setName(root.needs("name").get<std::string>());
    res.metadata.setDescription(root.needs("description").get<std::string>());
    res.metadata.setVersion(root.needs("version").get<VersionInfo>());
    res.metadata.setIsAPI(root.needs("api").get<bool>());

    std::vector<ModMetadata::Dependency> dependencies {};
    for (auto& obj : root.hasNullable("dependencies").items()) {
        // todo: this should probably be generalized to use the same function as mod.json

        bool onThisPlatform = !obj.hasNullable("platforms");
        for (auto& plat : obj.hasNullable("platforms").items()) {
            if (PlatformID::coveredBy(plat.get<std::string>(), GEODE_PLATFORM_TARGET)) {
                onThisPlatform = true;
            }
        }
        if (!onThisPlatform) {
            continue;
        }

        ModMetadata::Dependency dependency;
        obj.needs("mod_id").mustBe<std::string>("a valid id", &ModMetadata::validateID).into(dependency.id);
        obj.needs("version").into(dependency.version);
        obj.hasNullable("importance").into(dependency.importance);

        // Check if this dependency is installed, and if so assign the `mod` member to mark that
        auto mod = Loader::get()->getInstalledMod(dependency.id);
        if (mod && dependency.version.compare(mod->getVersion())) {
            dependency.mod = mod;
        }

        dependencies.push_back(dependency);
    }
    res.metadata.setDependencies(dependencies);

    std::vector<ModMetadata::Incompatibility> incompatibilities {};
    for (auto& obj : root.hasNullable("incompatibilities").items()) {
        ModMetadata::Incompatibility incompatibility;
        obj.hasNullable("importance").into(incompatibility.importance);

        auto modIdValue = obj.needs("mod_id");

        // Do not validate if we have a supersede, maybe the old ID is invalid
        if (incompatibility.importance == ModMetadata::Incompatibility::Importance::Superseded) {
            modIdValue.into(incompatibility.id);
        } else {
            modIdValue.mustBe<std::string>("a valid id", &ModMetadata::validateID).into(incompatibility.id);
        }

        obj.needs("version").into(incompatibility.version);

        // Check if this incompatability is installed, and if so assign the `mod` member to mark that
        auto mod = Loader::get()->getInstalledMod(incompatibility.id);
        if (mod && incompatibility.version.compare(mod->getVersion())) {
            incompatibility.mod = mod;
        }

        incompatibilities.push_back(incompatibility);
    }
    res.metadata.setIncompatibilities(incompatibilities);

    return root.ok(res);
}

Result<ServerModReplacement> ServerModReplacement::parse(matjson::Value const& raw) {
    auto root = checkJson(raw, "ServerModReplacement");
    auto res = ServerModReplacement();

    root.needs("id").into(res.id);
    root.needs("version").into(res.version);

    return root.ok(res);
}

Result<ServerModUpdate> ServerModUpdate::parse(matjson::Value const& raw) {
    auto root = checkJson(raw, "ServerModUpdate");

    auto res = ServerModUpdate();

    root.needs("id").into(res.id);
    root.needs("version").into(res.version);
    if (root.hasNullable("replacement")) {
        GEODE_UNWRAP_INTO(res.replacement, ServerModReplacement::parse(root.hasNullable("replacement").json()));
    }

    return root.ok(res);
}

Result<std::vector<ServerModUpdate>> ServerModUpdate::parseList(matjson::Value const& raw) {
    auto payload = checkJson(raw, "ServerModUpdatesList");

    std::vector<ServerModUpdate> list {};
    for (auto& item : payload.items()) {
        auto mod = ServerModUpdate::parse(item.json());
        if (mod) {
            list.push_back(mod.unwrap());
        }
        else {
            log::error("Unable to parse mod update from the server: {}", mod.unwrapErr());
        }
    }

    return payload.ok(list);
}

bool ServerModUpdate::hasUpdateForInstalledMod() const {
    if (auto mod = Loader::get()->getInstalledMod(this->id)) {
        return mod->getVersion() < this->version || this->replacement.has_value();
    }
    return false;
}

Result<ServerModLinks> ServerModLinks::parse(matjson::Value const& raw) {
    auto payload = checkJson(raw, "ServerModLinks");
    auto res = ServerModLinks();

    payload.hasNullable("community").into(res.community);
    payload.hasNullable("homepage").into(res.homepage);
    payload.hasNullable("source").into(res.source);

    return payload.ok(res);
}

Result<ServerModMetadata> ServerModMetadata::parse(matjson::Value const& raw) {
    auto root = checkJson(raw, "ServerModMetadata");

    auto res = ServerModMetadata();
    root.needs("id").into(res.id);
    root.needs("featured").into(res.featured);
    root.needs("download_count").into(res.downloadCount);
    root.hasNullable("about").into(res.about);
    root.hasNullable("changelog").into(res.changelog);
    root.hasNullable("repository").into(res.repository);
    if (root.has("created_at")) {
        GEODE_UNWRAP_INTO(res.createdAt, ServerDateTime::parse(root.has("created_at").get<std::string>()));
    }
    if (root.has("updated_at")) {
        GEODE_UNWRAP_INTO(res.updatedAt, ServerDateTime::parse(root.has("updated_at").get<std::string>()));
    }

    std::vector<std::string> developerNames;
    for (auto& obj : root.needs("developers").items()) {
        auto dev = ServerDeveloper();
        obj.needs("username").into(dev.username);
        obj.needs("display_name").into(dev.displayName);
        obj.needs("is_owner").into(dev.isOwner);
        res.developers.push_back(dev);
        developerNames.push_back(dev.displayName);
    }
    for (auto& item : root.needs("versions").items()) {
        auto versionRes = ServerModVersion::parse(item.json());
        if (versionRes) {
            auto version = versionRes.unwrap();
            version.metadata.setDetails(res.about);
            version.metadata.setChangelog(res.changelog);
            version.metadata.setDevelopers(developerNames);
            version.metadata.setRepository(res.repository);
            if (root.hasNullable("links")) {
                auto linkRes = ServerModLinks::parse(root.hasNullable("links").json());
                if (linkRes) {
                    auto links = linkRes.unwrap();
                    version.metadata.getLinksMut().getImpl()->m_community = links.community;
                    version.metadata.getLinksMut().getImpl()->m_homepage = links.homepage;
                    if (links.source.has_value()) version.metadata.setRepository(links.source);
                }
            }
            res.versions.push_back(version);
        }
        else {
            log::error("Unable to parse mod '{}' version from the server: {}", res.id, versionRes.unwrapErr());
        }
    }

    // Ensure there's at least one valid version
    if (res.versions.empty()) {
        return Err("Mod '{}' has no (valid) versions", res.id);
    }

    for (auto& item : root.hasNullable("tags").items()) {
        res.tags.insert(item.get<std::string>());
    }

    root.needs("download_count").into(res.downloadCount);

    return root.ok(res);
}

std::string ServerModMetadata::formatDevelopersToString() const {
    std::optional<ServerDeveloper> owner = ranges::find(developers, [] (auto item) {
        return item.isOwner;
    });
    switch (developers.size()) {
        case 0: return "Unknown"; break;
        case 1: return developers.front().displayName; break;
        case 2: return developers.front().displayName + " & " + developers.back().displayName; break;
        default: {
            if (owner) {
                return fmt::format("{} + {} More", owner->displayName, developers.size() - 1);
            } else {
                return fmt::format("{} + {} More", developers.front().displayName, developers.size() - 1);
            }
        } break;
    }
}

Result<ServerModsList> ServerModsList::parse(matjson::Value const& raw) {
    auto payload = checkJson(raw, "ServerModsList");

    auto list = ServerModsList();
    for (auto& item : payload.needs("data").items()) {
        auto mod = ServerModMetadata::parse(item.json());
        if (mod) {
            list.mods.push_back(mod.unwrap());
        }
        else {
            log::error("Unable to parse mod from the server: {}", mod.unwrapErr());
        }
    }
    payload.needs("count").into(list.totalModCount);

    return payload.ok(list);
}

ModMetadata ServerModMetadata::latestVersion() const {
    return this->versions.front().metadata;
}

bool ServerModMetadata::hasUpdateForInstalledMod() const {
    if (auto mod = Loader::get()->getInstalledMod(this->id)) {
        return mod->getVersion() < this->latestVersion().getVersion();
    }
    return false;
}

std::string server::getServerAPIBaseURL() {
    return "https://api.geode-sdk.org/v1";
}

template <class... Args>
std::string formatServerURL(fmt::format_string<Args...> fmt, Args&&... args) {
    return getServerAPIBaseURL() + fmt::format(fmt, std::forward<Args>(args)...);
}

std::string server::getServerUserAgent() {
    // no need to compute this more than once
    static const auto value = [] {
        // TODO: is this enough info? is it too much?
        return fmt::format("Geode Loader (ver={};commit={};platform={};gd={})",
            Loader::get()->getVersion().toNonVString(),
            about::getLoaderCommitHash(),
            GEODE_PLATFORM_SHORT_IDENTIFIER,
            LoaderImpl::get()->getGameVersion()
        );
    }();
    return value;
}

ServerRequest<ServerModsList> server::getMods(ModsQuery const& query, bool useCache) {
    if (useCache) {
        return getCache<getMods>().get(query);
    }

    auto req = web::WebRequest();
    req.userAgent(getServerUserAgent());

    // Add search params
    if (query.query) {
        req.param("query", *query.query);
    }

    req.param("gd", GEODE_GD_VERSION_STR);
    req.param("geode", Loader::get()->getVersion().toNonVString());

    if (query.platforms.size()) {
        std::string plats = "";
        bool first = true;
        for (auto plat : query.platforms) {
            if (!first) plats += ",";
            plats += PlatformID::toShortString(plat.m_value);
            first = false;
        }
        req.param("platforms", plats);
    }
    if (query.tags.size()) {
        req.param("tags", ranges::join(query.tags, ","));
    }
    if (query.featured) {
        req.param("featured", query.featured.value() ? "true" : "false");
    }
    req.param("sort", sortToString(query.sorting));
    if (query.developer) {
        req.param("developer", *query.developer);
    }

    // Paging (1-based on server, 0-based locally)
    req.param("page", std::to_string(query.page + 1));
    req.param("per_page", std::to_string(query.pageSize));

    return req.get(formatServerURL("/mods")).map(
        [](web::WebResponse* response) -> Result<ServerModsList, ServerError> {
            if (response->ok()) {
                // Parse payload
                auto payload = parseServerPayload(*response);
                if (!payload) {
                    return Err(payload.unwrapErr());
                }
                // Parse response
                auto list = ServerModsList::parse(payload.unwrap());
                if (!list) {
                    return Err(ServerError(response->code(), "Unable to parse response: {}", list.unwrapErr()));
                }
                return Ok(list.unwrap());
            }
            // Treat a 404 as empty mods list
            if (response->code() == 404) {
                return Ok(ServerModsList());
            }
            return Err(parseServerError(*response));
        },
        [](web::WebProgress* progress) {
            return parseServerProgress(*progress, "Downloading mods");
        }
    );
}

ServerRequest<ServerModMetadata> server::getMod(std::string const& id, bool useCache) {
    if (useCache) {
        return getCache<getMod>().get(id);
    }
    auto req = web::WebRequest();
    req.userAgent(getServerUserAgent());
    return req.get(formatServerURL("/mods/{}", id)).map(
        [](web::WebResponse* response) -> Result<ServerModMetadata, ServerError> {
            if (response->ok()) {
                // Parse payload
                auto payload = parseServerPayload(*response);
                if (!payload) {
                    return Err(payload.unwrapErr());
                }
                // Parse response
                auto list = ServerModMetadata::parse(payload.unwrap());
                if (!list) {
                    return Err(ServerError(response->code(), "Unable to parse response: {}", list.unwrapErr()));
                }
                return Ok(list.unwrap());
            }
            return Err(parseServerError(*response));
        },
        [id](web::WebProgress* progress) {
            return parseServerProgress(*progress, "Downloading metadata for " + id);
        }
    );
}

ServerRequest<ServerModVersion> server::getModVersion(std::string const& id, ModVersion const& version, bool useCache) {
    if (useCache) {
        auto& cache = getCache<getModVersion>();

        auto cachedRequest = cache.get(id, version);

        // if mod installation was cancelled, remove it from cache and fetch again
        if (cachedRequest.isCancelled()) {
            cache.remove(id, version);
            return cache.get(id, version);
        } else {
            return cachedRequest;
        }
    }

    auto req = web::WebRequest();
    req.userAgent(getServerUserAgent());

    std::string versionURL;
    std::visit(makeVisitor {
        [&](ModVersionLatest const&) {
            versionURL = "latest";
        },
        [&](ModVersionMajor const& ver) {
            versionURL = "latest";
            req.param("major", std::to_string(ver.major));
        },
        [&](ModVersionSpecific const& ver) {
            versionURL = ver.toNonVString();
        },
    }, version);

    return req.get(formatServerURL("/mods/{}/versions/{}?gd={}&platforms={}", id, versionURL, Loader::get()->getGameVersion(), GEODE_PLATFORM_SHORT_IDENTIFIER)).map(
        [](web::WebResponse* response) -> Result<ServerModVersion, ServerError> {
            if (response->ok()) {
                // Parse payload
                auto payload = parseServerPayload(*response);
                if (!payload) {
                    return Err(payload.unwrapErr());
                }
                // Parse response
                auto list = ServerModVersion::parse(payload.unwrap());
                if (!list) {
                    return Err(ServerError(response->code(), "Unable to parse response: {}", list.unwrapErr()));
                }
                return Ok(list.unwrap());
            }
            return Err(parseServerError(*response));
        },
        [id](web::WebProgress* progress) {
            return parseServerProgress(*progress, "Downloading metadata for " + id);
        }
    );
}

ServerRequest<ByteVector> server::getModLogo(std::string const& id, bool useCache) {
    if (useCache) {
        return getCache<getModLogo>().get(id);
    }
    auto req = web::WebRequest();
    req.userAgent(getServerUserAgent());
    return req.get(formatServerURL("/mods/{}/logo", id)).map(
        [](web::WebResponse* response) -> Result<ByteVector, ServerError> {
            if (response->ok()) {
                return Ok(response->data());
            }
            return Err(parseServerError(*response));
        },
        [id](web::WebProgress* progress) {
            return parseServerProgress(*progress, "Downloading logo for " + id);
        }
    );
}

ServerRequest<std::vector<ServerTag>> server::getTags(bool useCache) {
    if (useCache) {
        return getCache<getTags>().get();
    }
    auto req = web::WebRequest();
    req.userAgent(getServerUserAgent());
    return req.get(formatServerURL("/detailed-tags")).map(
        [](web::WebResponse* response) -> Result<std::vector<ServerTag>, ServerError> {
            if (response->ok()) {
                // Parse payload
                auto payload = parseServerPayload(*response);
                if (!payload) {
                    return Err(payload.unwrapErr());
                }
                auto list = ServerTag::parseList(payload.unwrap());
                if (!list) {
                    return Err(ServerError(response->code(), "Unable to parse response: {}", list.unwrapErr()));
                }
                return Ok(list.unwrap());
            }
            return Err(parseServerError(*response));
        },
        [](web::WebProgress* progress) {
            return parseServerProgress(*progress, "Downloading valid tags");
        }
    );
}

ServerRequest<std::optional<ServerModUpdate>> server::checkUpdates(Mod const* mod) {
    return checkAllUpdates().map(
        [mod](Result<std::vector<ServerModUpdate>, ServerError>* result) -> Result<std::optional<ServerModUpdate>, ServerError> {
            if (result->isOk()) {
                for (auto& update : result->unwrap()) {
                    if (
                        update.id == mod->getID() &&
                        (update.version > mod->getVersion() || update.replacement.has_value())
                    ) {
                        return Ok(update);
                    }
                }
                return Ok(std::nullopt);
            }
            return Err(result->unwrapErr());
        }
    );
}

ServerRequest<std::vector<ServerModUpdate>> server::batchedCheckUpdates(std::vector<std::string> const& batch) {
    auto req = web::WebRequest();
    req.userAgent(getServerUserAgent());
    req.param("platform", GEODE_PLATFORM_SHORT_IDENTIFIER);
    req.param("gd", GEODE_GD_VERSION_STR);
    req.param("geode", Loader::get()->getVersion().toNonVString());

    req.param("ids", ranges::join(batch, ";"));
    return req.get(formatServerURL("/mods/updates")).map(
        [](web::WebResponse* response) -> Result<std::vector<ServerModUpdate>, ServerError> {
            if (response->ok()) {
                // Parse payload
                auto payload = parseServerPayload(*response);
                if (!payload) {
                    return Err(payload.unwrapErr());
                }
                // Parse response
                auto list = ServerModUpdate::parseList(payload.unwrap());
                if (!list) {
                    return Err(ServerError(response->code(), "Unable to parse response: {}", list.unwrapErr()));
                }
                return Ok(list.unwrap());
            }
            return Err(parseServerError(*response));
        },
        [](web::WebProgress* progress) {
            return parseServerProgress(*progress, "Checking updates for mods");
        }
    );
}

void server::queueBatches(
    ServerRequest<std::vector<ServerModUpdate>>::PostResult const resolve,
    std::shared_ptr<std::vector<std::vector<std::string>>> const batches,
    std::shared_ptr<std::vector<ServerModUpdate>> accum
) {
    // we have to do the copy here, or else our values die
    batchedCheckUpdates(batches->back()).listen([resolve, batches, accum](auto result) {
        if (result->isOk()) {
            auto serverValues = result->unwrap();

            accum->reserve(accum->size() + serverValues.size());
            accum->insert(accum->end(), serverValues.begin(), serverValues.end());

            if (batches->size() > 1) {
                batches->pop_back();
                queueBatches(resolve, batches, accum);
            }
            else {
                resolve(Ok(*accum));
            }
        }
        else {
            if (result->isOk()) {
                resolve(Ok(result->unwrap()));
            }
            else {
                resolve(Err(result->unwrapErr()));
            }
        }
    });
}

ServerRequest<std::vector<ServerModUpdate>> server::checkAllUpdates(bool useCache) {
    if (useCache) {
        return getCache<checkAllUpdates>().get();
    }

    auto modIDs = ranges::map<std::vector<std::string>>(
        Loader::get()->getAllMods(),
        [](auto mod) { return mod->getID(); }
    );

    // if there's no mods, the request would just be empty anyways
    if (modIDs.empty()) {
        // you would think it could infer like literally anything
        return ServerRequest<std::vector<ServerModUpdate>>::immediate(
            Ok<std::vector<ServerModUpdate>>({})
        );
    }

    auto modBatches = std::make_shared<std::vector<std::vector<std::string>>>();
    auto modCount = modIDs.size();
    std::size_t maxMods = 200u; // this affects 0.03% of users

    if (modCount <= maxMods) {
        // no tricks needed
        return batchedCheckUpdates(modIDs);
    }

    // even out the mod count, so a request with 230 mods sends two 115 mod requests
    auto batchCount = modCount / maxMods + 1;
    auto maxBatchSize = modCount / batchCount + 1;

    for (std::size_t i = 0u; i < modCount; i += maxBatchSize) {
        auto end = std::min(modCount, i + maxBatchSize);
        modBatches->emplace_back(modIDs.begin() + i, modIDs.begin() + end);
    }

    // chain requests to avoid doing too many large requests at once
    return ServerRequest<std::vector<ServerModUpdate>>::runWithCallback(
        [modBatches](auto finish, auto progress, auto hasBeenCancelled) {
            auto accum = std::make_shared<std::vector<ServerModUpdate>>();
            queueBatches(finish, modBatches, accum);
        },
        "Mod Update Check"
    );
}

void server::clearServerCaches(bool clearGlobalCaches) {
    getCache<&getMods>().clear();
    getCache<&getMod>().clear();
    getCache<&getModLogo>().clear();

    // Only clear global caches if explicitly requested
    if (clearGlobalCaches) {
        getCache<&getTags>().clear();
        getCache<&checkAllUpdates>().clear();
    }
}

$on_mod(Loaded) {
    listenForSettingChanges<int64_t>("server-cache-size-limit", +[](int64_t size) {
        getCache<&server::getMods>().limit(size);
        getCache<&server::getMod>().limit(size);
        getCache<&server::getModLogo>().limit(size);
        getCache<&server::getTags>().limit(size);
        getCache<&server::checkAllUpdates>().limit(size);
    });
}
