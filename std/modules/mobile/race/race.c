/**
 * @file /std/user/body/race/race.c
 * @description Race body module
 *
 * @created 2024-07-25 - Gesslar
 * @last_modified 2024-07-25 - Gesslar
 *
 * @history
 * 2024-07-25 - Gesslar - Created
 */

inherit DIR_STD_MODULES_MOBILE "module" ;

string query_race() ;
int start_module(object ob, mixed args...) ;
protected int use_default_body_parts() ;
void wipe_body_parts() ;
int add_body_part(string part, int size, int vitalness) ;
int remove_body_part(string part) ;
mapping query_body_parts() ;
string *query_body_part_names() ;
int query_body_part_size(string part) ;
int query_body_part_vitalness(string part) ;
int set_body_part_size(string part, int size) ;
int add_body_part_size(string part, int size_mod) ;
int set_body_part_vitalness(string part, int vitalness) ;
int add_body_part_vitalness(string part, int vitalness_mod) ;
int query_body_part_size_modifier(string part) ;
int query_body_part_vitalness_modifier(string part) ;
varargs int valid_body_part(string part, int force) ;
string random_body_part() ;

int add_equipment_slot(string slot, string *parts) ;
mapping query_equipment_slots() ;
string *query_equipment_slot(string slot) ;
int remove_equipment_slot(string slot) ;
int valid_equipment_slot(string slot) ;
int covers_body_part(string slot, string part) ;
string covered_by_slot(string part) ;

protected nosave string race ;
protected nosave mapping regen_rate = ([ "hp" : 0.0, "sp" : 0.0, "mp" : 0.0 ]) ;

// This is a mapping of body parts and their target sizes on the body.
// Each body part has an array of two integers. The first integer is the
// size of the body part, and the second integer vitalness of the body part.
//
// Size controls how likely it is to be selected when a random body part is
// requested.
protected nosave mapping default_body_parts = ([
    "head"      : ({  3, 3 }),
    "neck"      : ({  2, 2 }),
    "torso"     : ({ 30, 1 }),
    "left arm"  : ({  4, 1 }),
    "right arm" : ({  4, 1 }),
    "left hand" : ({  2, 1 }),
    "right hand": ({  2, 1 }),
    "left leg"  : ({  4, 1 }),
    "right leg" : ({  4, 1 }),
    "left foot" : ({  2, 1 }),
    "right foot": ({  2, 1 }),
]) ;

protected nosave mapping default_equipment_slots = ([
    "head"     : ({"head"}),
    "neck"     : ({"neck"}),
    "torso"    : ({"torso"}),
    "arms"     : ({"left arm", "right arm"}),
    "hands"    : ({"left hand", "right hand"}),
    "legs"     : ({"left leg", "right leg"}),
    "feet"     : ({"left foot", "right foot"}),
]) ;

protected nosave mapping body_parts = ([ ]) ;
protected nosave mapping body_part_vitalness = ([ ]) ;
protected nosave mapping body_part_modifiers = ([ ]) ;

protected nosave mapping equipment_slots = ([ ]) ;

private string module_name;

void setup() {
    module_name = "race" ;
}

string query_race() {
    return replace_string(query_file_name(), "_", " ") ;
}

/**
 * 
 * @param {STD_PLAYER} ob 
 * @param args 
 */
int start_module(object ob, mixed args...) {
    int result ;

    if(ob->query_module(module_name))
        return 0 ;

    catch(result = call_if(this_object(), "set_up_body_parts", ob, args...)) ;

    return result ;
}

protected int use_default_body_parts() {
    wipe_body_parts() ;
    foreach(string part, int *values in default_body_parts)
        if(!add_body_part(part, values[0], values[1]))
            return 0 ;

    foreach(string slot, string *parts in default_equipment_slots)
        if(!add_equipment_slot(slot, parts))
            return 0 ;

    return 1 ;
}

void wipe_body_parts() {
    body_parts = ([ ]) ;
    body_part_vitalness = ([ ]) ;
    body_part_modifiers = ([ ]) ;
    equipment_slots = ([ ]) ;
}

int add_body_part(string part, int size, int vitalness) {
    if(valid_body_part(part, 1))
        return 0 ;

    body_parts[part] = size ;
    body_part_vitalness[part] = vitalness ;
    body_part_modifiers[part] = ([ ]) ;

    return 1 ;
}

int remove_body_part(string part) {
    if(!valid_body_part(part))
        return 0 ;

    map_delete(body_parts, part) ;
    map_delete(body_part_vitalness, part) ;
    map_delete(body_part_modifiers, part) ;

    return 1 ;
}

mapping query_body_parts() {
    return copy(body_parts) ;
}

string *query_body_part_names() {
    return keys(body_parts) ;
}

int query_body_part_size(string part) {
    if(!valid_body_part(part))
        return 0 ;

    return body_parts[part][0] ;
}

int query_body_part_vitalness(string part) {
    if(!valid_body_part(part))
        return 0 ;

    return body_part_vitalness[part] ;
}

int set_body_part_size(string part, int size) {
    if(!valid_body_part(part))
        return 0 ;

    if(size < 1) size = 1 ;

    return body_parts[part] = size ;
}

int add_body_part_size(string part, int size_mod) {
    int new_size_mod ;

    if(!valid_body_part(part))
        return 0 ;

    // determine if the adjusted size would be blow 1, and if so
    // change size to be what would bring it to 1
    if(body_parts[part] + size_mod < 1) {
        new_size_mod = 1 - body_parts[part] ;
    } else {
        new_size_mod = size_mod ;
    }

    body_parts[part] += new_size_mod ;

    return new_size_mod ;
}

int set_body_part_vitalness(string part, int vitalness) {
    if(!valid_body_part(part))
        return 0 ;

    if(vitalness < 1)
        vitalness = 1 ;

    return body_part_vitalness[part] = vitalness ;
}

int add_body_part_vitalness(string part, int vitalness_mod) {
    int new_vitalness_mod ;

    if(!valid_body_part(part))
        return 0 ;

    // determine if the adjusted vitalness would be blow 1, and if so
    // change vitalness to be what would bring it to 1
    if(body_part_vitalness[part] + vitalness_mod < 1) {
        new_vitalness_mod = 1 - body_part_vitalness[part] ;
    } else {
        new_vitalness_mod = vitalness_mod ;
    }

    body_part_vitalness[part] += new_vitalness_mod ;

    return new_vitalness_mod ;
}

int query_body_part_size_modifier(string part) {
    return 0 ;
}

int query_body_part_vitalness_modifier(string part) {
    return 0 ;
}

/**
 * @function valid_body_part
 * @description Check if a body part is valid. It is valid if it is a string
 * and is a key in the body_parts mapping. Force is an optional argument that
 * will force the function to forgo the check for membership in the body_parts
 * @param {string} part - the body part to check
 * @param {int} [force=0] force - optional, default 0
 * @returns {int} - 1 if the body part is valid, 0 otherwise
 */
int valid_body_part(string part, int force: (: 0 :)) {
    if(!stringp(part))
        return 0 ;

    return of(part, body_parts) ;
}

string random_body_part() {
    return element_of_weighted(body_parts) ;
}

int add_equipment_slot(string slot, string *parts) {
    if(of(slot, equipment_slots))
        return 0 ;

    equipment_slots[slot] = parts ;

    return 1 ;
}

mapping query_equipment_slots() {
    return copy(equipment_slots) ;
}

string *query_equipment_slot(string slot) {
    if(!of(slot, equipment_slots))
        return 0 ;

    return equipment_slots[slot] ;
}

int remove_equipment_slot(string slot) {
    if(!of(slot, equipment_slots))
        return 0 ;

    map_delete(equipment_slots, slot) ;

    return 1 ;
}

int valid_equipment_slot(string slot) {
    return of(slot, equipment_slots) ;
}

int covers_body_part(string slot, string part) {
    if(!valid_equipment_slot(slot))
        return 0 ;

    return member_array(part, equipment_slots[slot]) != -1 ;
}

string covered_by_slot(string part) {
    foreach(string slot, string *parts in equipment_slots)
        if(member_array(part, parts) != -1)
            return slot ;

    return 0 ;
}

varargs mapping query_regen_rate(string type) {
    if(type)
        return regen_rate[type] ;

    return copy(regen_rate) ;
}
