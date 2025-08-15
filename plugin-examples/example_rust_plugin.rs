use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::ptr;

// FFI bindings for the C wrapper
#[repr(C)]
pub struct CfCallbackInfo {
    module_args: *const CfModuleArgs,
    parse_args: *mut c_void,
}

#[repr(C)]
pub struct CfModuleArgs {
    prev: *mut CfModuleArgs,
    name: *mut c_char,
    value: *mut c_char,
    next: *mut CfModuleArgs,
}

type CfModuleHandler = extern "C" fn(*const CfCallbackInfo) -> *const c_char;

#[link(name = "cufetch_c")]
extern "C" {
    fn cf_config_get_string(config: *mut c_void, path: *const c_char, fallback: *const c_char) -> *mut c_char;
    fn cf_create_module(name: *const c_char, description: *const c_char, handler: CfModuleHandler) -> *mut c_void;
    fn cf_add_submodule(parent: *mut c_void, child: *mut c_void) -> bool;
    fn cf_register_module_tree(root: *mut c_void) -> bool;
    fn cf_free_string(str: *mut c_char);
    fn cf_find_arg_by_name(info: *const CfCallbackInfo, name: *const c_char) -> *const CfModuleArgs;
    fn cf_get_config_from_callback(info: *const CfCallbackInfo, path: *const c_char, fallback: *const c_char) -> *mut c_char;
}

// Static storage for plugin data
static mut USERNAME: Option<String> = None;

// Module handlers
extern "C" fn rust_user_name_handler(callback_info: *const CfCallbackInfo) -> *const c_char {
    unsafe {
        let username = USERNAME.as_ref().unwrap_or(&"unknown".to_string());
        
        // Get custom format from config
        let path = CString::new("plugin.rust-user.format").unwrap();
        let fallback = CString::new("default").unwrap();
        let format_ptr = cf_get_config_from_callback(callback_info, path.as_ptr(), fallback.as_ptr());
        
        let format = if !format_ptr.is_null() {
            let format_cstr = CStr::from_ptr(format_ptr);
            format_cstr.to_string_lossy().into_owned()
        } else {
            "default".to_string()
        };
        
        if !format_ptr.is_null() {
            cf_free_string(format_ptr);
        }
        
        let result = format!("{} ({})", username, format);
        CString::new(result).unwrap().into_raw()
    }
}

extern "C" fn rust_user_language_handler(_callback_info: *const CfCallbackInfo) -> *const c_char {
    CString::new("Rust").unwrap().into_raw()
}

extern "C" fn rust_user_status_handler(_callback_info: *const CfCallbackInfo) -> *const c_char {
    CString::new("Learning customfetch plugins").unwrap().into_raw()
}

// Plugin lifecycle functions
#[no_mangle]
pub extern "C" fn start(handle: *mut c_void, config: *const c_void) {
    unsafe {
        
        // Get username from config
        let path = CString::new("plugin.rust-user.username").unwrap();
        let fallback = CString::new("rustacean").unwrap();
        let username_ptr = cf_config_get_string(ptr::null_mut(), path.as_ptr(), fallback.as_ptr());
        
        if !username_ptr.is_null() {
            let username_cstr = CStr::from_ptr(username_ptr);
            USERNAME = Some(username_cstr.to_string_lossy().into_owned());
            cf_free_string(username_ptr);
        }
        
        // Create submodules
        let name_desc = CString::new("Rust user name").unwrap();
        let lang_desc = CString::new("Favorite language").unwrap();
        let status_desc = CString::new("Current status").unwrap();
        let parent_desc = CString::new("Rust user information").unwrap();
        
        let name_module = cf_create_module(
            CString::new("name").unwrap().as_ptr(),
            name_desc.as_ptr(),
            rust_user_name_handler
        );
        
        let language_module = cf_create_module(
            CString::new("language").unwrap().as_ptr(),
            lang_desc.as_ptr(),
            rust_user_language_handler
        );
        
        let status_module = cf_create_module(
            CString::new("status").unwrap().as_ptr(),
            status_desc.as_ptr(),
            rust_user_status_handler
        );
        
        // Create parent module
        let rust_user_module = cf_create_module(
            CString::new("rust-user").unwrap().as_ptr(),
            parent_desc.as_ptr(),
            ptr::null_mut()
        );
        
        // Build hierarchy
        if cf_add_submodule(rust_user_module, name_module) &&
           cf_add_submodule(rust_user_module, language_module) &&
           cf_add_submodule(rust_user_module, status_module) {
            
            if !cf_register_module_tree(rust_user_module) {
                eprintln!("Error: Failed to register Rust module tree");
            }
        } else {
            eprintln!("Error: Failed to add submodules");
        }
    }
}

#[no_mangle]
pub extern "C" fn finish(_handle: *mut c_void) {
    unsafe {
        USERNAME = None;
    }
}
