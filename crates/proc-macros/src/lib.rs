#![allow(unused_variables)]

use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, DeriveInput, ItemFn};

#[proc_macro_attribute]
pub fn virtual_cpp(attr: TokenStream, item: TokenStream) -> TokenStream {
    let input = parse_macro_input!(item as ItemFn);
    let vis = &input.vis;
    let name = &input.sig.ident;
    let args = &input.sig.inputs;
    let ret = &input.sig.output;
    let body = &input.block;

    let expanded = quote! {
        trait VirtualFn {
            fn #name(&self, $($args),*) #ret;
        }

        impl<T: VirtualFn + ?Sized> VirtualFn for Box<T> {
            fn #name(&self, $($args),*) #ret {
                (**self).#name($(#args),*)
            }
        }

        impl<T: VirtualFn + ?Sized> VirtualFn for &T {
            fn #name(&self, $($args),*) #ret {
                (**self).#name($(#args),*)
            }
        }

        impl<T: VirtualFn + ?Sized> VirtualFn for &mut T {
            fn #name(&self, $($args),*) #ret {
                (**self).#name($(#args),*)
            }
        }

        impl VirtualFn for Box<dyn VirtualFn> {
            fn #name(&self, $($args),*) #ret {
                (**self).#name($(#args),*)
            }
        }

        impl VirtualFn for &dyn VirtualFn {
            fn #name(&self, $($args),*) #ret {
                (**self).#name($(#args),*)
            }
        }

        impl VirtualFn for &mut dyn VirtualFn {
            fn #name(&self, $($args),*) #ret {
                (**self).#name($(#args),*)
            }
        }

        struct _VirtualFnImpl;
        impl _VirtualFnImpl {
            #vis fn new() -> Box<dyn VirtualFn> {
                Box::new(_VirtualFnImpl)
            }
        }

        impl VirtualFn for _VirtualFnImpl {
            fn #name(&self, $($args),*) #ret {
                #body
            }
        }
    };

    TokenStream::from(expanded)
}

#[proc_macro_attribute]
pub fn static_cpp(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let input = parse_macro_input!(item as ItemFn);
    let ident = &input.sig.ident;
    let expanded = quote! {
        #[inline]
        pub const fn #ident() -> Self {
            Self
        }
    };

    TokenStream::from(expanded)
}

#[proc_macro_attribute]
pub fn concept(_: TokenStream, input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);
    let name = &input.ident;

    let expanded = quote! {
        trait #name {}

        impl #name for #name {}
    };

    expanded.into()
}
