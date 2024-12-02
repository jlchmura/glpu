#include <simul_efun.h>

//user.c

//Tacitus @ LPUniversity
//Grouped on October 22nd, 2005

/**
 * @simul_efun user_data_file
 * @description Returns the file path for the user's data file.
 * @param {string} name - The user's name.
 * @returns {string} - The file path for the user's data file, or 0 if the input
 *                     is invalid.
 */
string user_data_file(string name) {
    if(!name ||!stringp(name)) return 0 ;
    name = lower_case(name) ;
    return("/data/users/" + name[0..0] + "/" + name + "/" + name) ;
}

/**
 * @simul_efun user_body_data
 * @description Returns the file path for the user's mob data file.
 * @param {string} name - The user's name.
 * @returns {string} - The file path for the user's mob data file, or 0 if the input
 *                     is invalid.
 */
string user_body_data(string name) {
    if(!name ||!stringp(name)) return 0 ;
    name = lower_case(name) ;
    return("/data/users/" + name[0..0] + "/" + name + "/" + name) ;
}

/**
 * @simul_efun user_inventory_data
 * @description Returns the file path for the user's inventory data file.
 * @param {string} name - The user's name.
 * @returns {string} - The file path for the user's inventory data file, or 0
 *                     if the input is invalid.
 */
string user_inventory_data(string name) {
    if(!name ||!stringp(name)) return 0 ;
    name = lower_case(name) ;
    return user_data_directory(name) + name + "_inv.txt" ;
}

/**
 * @simul_efun user_data_directory
 * @description Returns the directory path for the user's data directory based on
 *              their name.
 * @param {string} name - The user's name.
 * @returns {string} - The directory path for the user's data directory, or 0 if the
 *                     input is invalid.
 */
string user_data_directory(string name) {
    if(!name ||!stringp(name)) return 0 ;
    name = lower_case(name) ;
    return("/data/users/" + name[0..0] + "/" + name + "/") ;
}

/**
 * @simul_efun home_path
 * @description Returns the home directory path for the user.
 * @param {string|object} name - The user's name, or the body of the user.
 * @returns {string} - The home directory path for the user.
 */
string home_path(mixed name) {
    if(objectp(name) && userp(name)) name = query_privs(name) ;
    if(!stringp(name)) return null ;

    return "/home/" + name[0..0] + "/" + name + "/" ;
}

/**
 * @simul_efun account_path
 * @description Returns the path for the account file.
 * @param {string} name - The name of the account.
 * @returns {string} - The path for the account file, or 0 if the input is invalid.
 */
string account_path(string name) {
    if(!name ||!stringp(name)) return 0 ;
    name = lower_case(name) ;
    return("/data/accounts/" + name[0..0] + "/") ;
}

/**
 * @simul_efun account_file
 * @description Returns the file path for the account file.
 * @param {string} name - The name of the account.
 * @returns {string} - The file path for the account file, or 0 if the input is invalid.
 */
string account_file(string name) {
    return account_path(name) + name + ".txt" ;
}

/**
 * @simul_efun valid_account
 * @description Returns 1 if the account is valid, 0 if not.
 * @param {string} name - The name of the account.
 * @returns {int} - 1 if the account is valid, otherwise 0.
 */
int valid_account(string name) {
    return mapp(ACCOUNT_D->load_account(name)) ;
}

/**
 * @simul_efun ghostp
 * @description Returns 1 if the object is a ghost, 0 if not.
 * @param {STD_GHOST} ob - The object to check.
 * @returns {int} - 1 if the object is a ghost, otherwise 0.
 */
int ghostp(object ob) {
    return ob->is_ghost() ;
}

/**
 * @simul_efun linkp
 * @description Returns 1 if the object is a user object, 0 if not.
 * @param {object} ob - The object to check.
 * @returns {int} - 1 if the object is a user object, otherwise 0.
 */
int linkp(object ob) {    
    return ob->is_user() ;
}

/**
 * @simul_efun valid_user
 * @description Returns 1 if the object is a valid user, 0 if not.
 * @param {mixed} ob - The object to check.
 * @returns {int} - 1 if the object is a valid user, otherwise 0.
 */
varargs int valid_user(mixed ob) {
    if(nullp(ob))
        ob = previous_object() ;

    if(objectp(ob))
        ob = query_privs(ob) ;

    if(!stringp(ob))
        return 0 ;

    return file_exists(user_data_file(lower_case(ob)) + __SAVE_EXTENSION__) ;
}
