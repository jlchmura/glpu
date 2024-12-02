/**
 * @file /std/modules/shop_menu.c
 * @description A module to enable a shop that uses a menu instead of an
 *              inventory.
 *
 * @created 2024-08-17 - Gesslar
 * @last_modified 2024-08-17 - Gesslar
 *
 * @history
 * 2024-08-17 - Gesslar - Created
 */

#include <classes.h>
#include "/std/object/include/command.h"
#include "/std/object/include/object.h"

inherit CLASS_MENU ;
inherit M_CURRENCY ;

private nosave class Menu *food_menu = ({ }) ;

void reset_menu() ;
void init_shop() ;
void add_menu_item(string type, string file, mixed *cost) ;
void remove_menu_item(string file) ;
void wipe_menu() ;

void init_shop() {
    add_command("buy", "cmd_buy") ;
    add_command(({"list","menu"}), "cmd_menu") ;
    add_command("view", "cmd_view") ;
}

/**
 * @description Add a food item to the menu.
 * @param {string} type - The type of the item.
 * @param {string} file - The file to add to the menu.
 * @param {mixed*} cost - The cost of the item.
 */
varargs void add_menu_item(string type, string file, int cost) {
    class Menu item ;
    /** @type {STD_ITEM} */
    object ob ;
    string err ;

    if(nullp(type) || nullp(file))
        return ;

    err = catch(ob = load_object(file)) ;
    if(err)
        return ;

    if(!cost)
        cost = ob->query_value() ;

    item = new(class Menu,
        type : type,
        file : file,
        short: ob->query_short(),
        id   : ob->query_ids(),
        adj  : ob->query_adjs(),
        description : ob->query_long(),
        cost : cost
    ) ;

    food_menu += ({ item }) ;
}

/**
 * @description Remove a food item from the menu.
 * @param {string} file - The file to remove from the menu.
 */
void remove_menu_item(string file) {
    int sz, pos ;

    sz = sizeof(food_menu) ;

    for(pos = 0; pos < sz; pos++) {
        if(food_menu[pos]->file == file) {
            remove_array_element(food_menu, pos) ;
            break ;
        }
    }

}

/**
 * @description Player command to list the menu.
 * @param {object} tp - The player object.
 * @param {string} str - The type of the item to list.
 * @return {mixed} - The menu list.
 */
mixed cmd_menu(object tp, string str) {
    class Menu *items, item ;
    string out ;

    out = get_short() + "\n\n" ;

    items = food_menu ;

    if(str)
        items = filter(items, (: $1.type == $(str) :)) ;

    if(sizeof(items) == 0)
        return out + "No items for sale." ;

    foreach(item in items) {
        out += sprintf("%s (%d)\n",
            item.short,
            item.cost
        ) ;
    }

    return out ;
}

/**
 * @description Player command to buy an item.
 * @param {STD_PLAYER} tp - The player object.
 * @param {string} str - The item to buy.
 * @return {mixed} - The result of the buy command.
 */
mixed cmd_buy(object tp, string str) {
    /** @type {STD_ITEM} */
    object ob ;
    string file ;
    string err ;
    mixed result ;
    string action ;
    mixed *paid, *change ;
    class Menu item ;
    int found ;

    if(!str)
        return "What do you want to buy?" ;

    foreach(item in food_menu) {
        if(of(str, item.id)) {
            found = 1 ;
            break ;
        }
    }

    if(!found)
        return "No such item for sale." ;

    result = handle_transaction(tp, item.cost) ;
    if(stringp(result))
        return result ;

    err = catch(ob = new(item.file)) ;
    if(err) {
        reverse_transaction(tp, result) ;
        return "No such item for sale." ;
    }

    if(ob->move(tp)) {
        reverse_transaction(tp, result) ;
        ob->remove() ;
        return "You can't carry that much weight." ;
    }

    tp->other_action("$N $vbuy a $o.", ob) ;

    paid = result[0] ;
    change = result[1] ;

    action = "$N $vbuy a $o for $o1" ;
    if(sizeof(change))
        action += " and receive $o2 in change" ;

    action += "." ;

    tp->my_action(action,
        get_short(ob),
        format_return_currency_string(paid),
        format_return_currency_string(change)
    ) ;

    return 1 ;
}

/**
 * @description Player command to view an item.
 * @param {object} tp - The player object.
 * @param {string} str - The item to view.
 * @return {mixed} - The result of the view command.
 */
mixed cmd_view(object tp, string str) {
    class Menu item ;

    if(!str)
        return "What do you want to view?" ;

    foreach(item in food_menu) {
        if(of(str, item.id))
            return sprintf("%s\n%s", item.short, item.description) ;
    }

    return "No such item for sale." ;
}

/**
 * @description Wipe the menu.
 */
void wipe_menu() {
    food_menu = ({ }) ;
}
