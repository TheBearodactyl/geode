use std::fmt;

use serde_json::{Map, Value};

#[derive(Debug)]
pub struct JsonError(String);

impl fmt::Display for JsonError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "JSON Error: {}", self.0)
    }
}

pub type JsonResult<T> = Result<T, JsonError>;
pub type JsonValue = serde_json::Value;

pub fn json_value_type_to_string(value: &JsonValue) -> String {
    match value {
        Value::Null => "null".to_string(),
        Value::Object(_) => "object".to_string(),
        Value::Array(_) => "array".to_string(),
        Value::String(_) => "string".to_string(),
        Value::Bool(_) => "boolean".to_string(),
        Value::Number(_) => "number".to_string(),
    }
}

pub fn json_convertible_to(value: &JsonValue, to: &JsonValue) -> bool {
    match (value, to) {
        (Value::Number(_), Value::Number(_)) => true,
        _ => value == to,
    }
}

pub type JsonValueValidator<T> = fn(&T) -> bool;

pub struct JsonMaybeSomething<'a> {
    pub m_json: &'a mut JsonValue,
    pub m_hierarchy: String,
    pub m_has_value: bool,
}

impl<'a> JsonMaybeSomething<'a> {
    pub fn new(json: &'a mut JsonValue, hierarchy: String, has_value: bool) -> Self {
        JsonMaybeSomething {
            m_json: json,
            m_hierarchy: hierarchy,
            m_has_value: has_value,
        }
    }

    pub fn is_error(&self) -> bool {
        matches!(*self.m_json, Value::Null)
    }

    pub fn get_error(&self) -> String {
        match self.m_json {
            Value::String(ref s) => s.clone(),
            _ => "".to_string(),
        }
    }

    pub fn as_bool(&mut self) -> JsonResult<bool> {
        match self.m_json {
            Value::Bool(b) => Ok(*b),
            _ => Err(JsonError(format!(
                "{}: Invalid type \"{}\", expected \"boolean\"",
                self.m_hierarchy,
                json_value_type_to_string(self.m_json)
            ))),
        }
    }

    pub fn as_f64(&mut self) -> JsonResult<f64> {
        match self.m_json {
            Value::Number(n) => Ok(n.as_f64().unwrap_or_default()),
            _ => Err(JsonError(format!(
                "{}: Invalid type \"{}\", expected \"number\"",
                self.m_hierarchy,
                json_value_type_to_string(self.m_json)
            ))),
        }
    }

    pub fn as_string(&mut self) -> JsonResult<String> {
        match self.m_json {
            Value::String(s) => Ok(s.clone()),
            _ => Err(JsonError(format!(
                "{}: Invalid type \"{}\", expected \"string\"",
                self.m_hierarchy,
                json_value_type_to_string(self.m_json)
            ))),
        }
    }

    pub fn as_array(&mut self) -> JsonResult<&Vec<JsonValue>> {
        match self.m_json {
            Value::Array(arr) => Ok(arr),
            _ => Err(JsonError(format!(
                "{}: Invalid type \"{}\", expected \"array\"",
                self.m_hierarchy,
                json_value_type_to_string(self.m_json)
            ))),
        }
    }

    pub fn into_array(self) -> JsonResult<Vec<JsonValue>> {
        match *self.m_json {
            Value::Array(ref mut arr) => Ok(arr.clone()),
            _ => Err(JsonError(format!(
                "{}: Invalid type \"{}\", expected \"array\"",
                self.m_hierarchy,
                json_value_type_to_string(self.m_json)
            ))),
        }
    }

    pub fn into_object(self) -> JsonResult<Map<String, JsonValue>> {
        match *self.m_json {
            Value::Object(ref mut obj) => Ok(obj.clone()),
            _ => Err(JsonError(format!(
                "{}: Invalid type \"{}\", expected \"object\"",
                self.m_hierarchy,
                json_value_type_to_string(self.m_json)
            ))),
        }
    }

    pub fn into_string(self) -> JsonResult<String> {
        match *self.m_json {
            Value::String(ref mut s) => Ok(s.clone()),
            _ => Err(JsonError(format!(
                "{}: Invalid type \"{}\", expected \"string\"",
                self.m_hierarchy,
                json_value_type_to_string(self.m_json)
            ))),
        }
    }
}

pub struct JsonChecker {
    pub m_json: JsonValue,
}

impl JsonChecker {
    pub fn new(json: JsonValue) -> Self {
        JsonChecker { m_json: json }
    }

    pub fn root(&mut self, hierarchy: &str) -> JsonMaybeSomething {
        JsonMaybeSomething::new(&mut self.m_json, hierarchy.to_string(), true)
    }
}

