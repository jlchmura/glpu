/* object.c

 Tacitus @ LPUniversity
 04-APR-05
 Base Object

*/

/* Last updated July 14th, 2006 by Tacitus */

#include <object.h>

inherit STD_OB_E;

inherit __DIR__ "description" ;
inherit __DIR__ "heartbeat" ;
inherit __DIR__ "id" ;
inherit __DIR__ "init" ;
inherit __DIR__ "persist" ;
inherit __DIR__ "setup" ;
inherit __DIR__ "stats" ;
inherit __DIR__ "weight" ;

inherit M_CLEAN ;
inherit M_MESSAGING ;

private nomask nosave function *destruct_functions = ({}) ;
private nomask nosave function *reset_functions = ({}) ;
private string real_name, short, long;
private nosave string name ;
protected nosave mixed *create_args = ({}) ;
private nosave string virtual_master = 0;

// Private so only driver can call it.
private varargs void create(mixed args...) {
    set_notify_destruct(1) ;
    init_ob() ;

    create_args = args ;
    if(!real_name) {
        if(name)
            set_real_name(name) ;
        else
            name = null ;
    }

    setup_chain(create_args...) ;
    reset() ;
}

void virtual_start() {
    virtual_setup_chain(create_args...) ;
    reset() ;
}

varargs void reset() {
    call_if(this_object(), "reset_objects") ;
    process_reset() ;
}

int remove() {
    object *obs, ob, next, env = environment();

    catch(call_if(this_object(), "removing", env));

    ob = first_inventory(this_object()) ;
    while(ob) {
        next = next_inventory(ob);

        if(env) {
            if(!ob->move(env)) {
                ob = next;
                continue;
            }
        }

        if(!userp(ob)) {
            ob->remove();
            if(objectp(ob))
                destruct(ob);
        }

        ob = next;
    }

    obs = all_inventory(this_object()) ;
    obs = filter(obs, (: !userp($1) :)) ;

    if(sizeof(obs))
        filter(obs, (: destruct :));

    destruct();

    return 1;
}

string set_real_name(string str) {
    if(interactive(this_object()) && !is_member(query_privs(previous_object()), "admin")
        && previous_object() != this_object()) return 0;

    real_name = lower_case(str);

    if(living())
        set_living_name(real_name);

    return real_name ;
}

string query_real_name() {
    return real_name ;
}

string set_name(string str) {
    string result ;

    if(!str)
        return 0;

    if(valid_user(lower_case(str)))
        name = capitalize(lower_case(str)) ;
    else
        name = str ;

    if(!real_name) {
        result = set_real_name(name) ;

        if(!result)
            return 0;
    }

    rehash_ids() ;

    return name ;
}

string query_name() {
    return name ;
}

string find_path(string path) {
    return resolve_path(query_directory(), path) ;
}

void set_virtual_master(string str) {
    virtual_master = str;
}

string query_virtual_master() {
    return virtual_master;
}

void on_destruct() {
    object env = environment() ;

    if(env) {
        if(!env->ignore_capacity())
            env->adjust_fill(-query_mass());
        if(!env->ignore_mass())
            env->adjust_mass(query_mass());
    }

    process_destruct() ;
    unsetup_chain() ;
}

void add_reset(function f) {
    if(valid_function(f) && !of(f, reset_functions)) {
        reset_functions += ({ f }) ;
    }
}

void remove_reset(function f) {
    if(of(f, reset_functions)) {
        reset_functions -= ({ f }) ;
    }
}

void process_reset() {
    foreach(function f in reset_functions) {
        catch(evaluate(f)) ;
    }
}

int add_destruct(function f) {
    if(valid_function(f) && !of(f, destruct_functions)) {
        destruct_functions += ({ f }) ;
        return 1 ;
    }

    return 0 ;
}

int remove_destruct(function f) {
    if(of(f, destruct_functions)) {
        destruct_functions -= ({ f }) ;
        return 1 ;
    }

    return 0 ;
}

void process_destruct() {
    foreach(function f in destruct_functions) {
        catch(evaluate(f)) ;
    }
}
