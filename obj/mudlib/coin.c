/**
 * @file /obj/mudlib/coin.c
 * @description A coin object
 *
 * @created 2024-02-19 - Gesslar
 * @last_modified 2024-08-08 - Gesslar
 *
 * @history
 * 2024-02-19 - Gesslar - Created
 * 2024-08-08 - Gesslar - Nearly completely rewritten and move logic added
 */

inherit STD_ITEM ;

string coin_type ;
int coin_num ;
int value ;

void set_up(string type, int num) ;
int adjust_coin_num(int num) ;

varargs void mudlib_setup(string type, int num) {
    if(!nullp(type) && !nullp(num) && num > 0)
        set_up(type, num) ;
    else
        set_up(null, null) ;
}

void update_descriptions() {
    if(coin_num == 1) {
        set_id(({ "coin", coin_type })) ;
        set_short(coin_type + " coin") ;
        set_long("A " + coin_type + " coin.") ;
    } else {
        set_id(({ "coins", coin_type + " coins" })) ;
        set_short(coin_num + " " + coin_type + " coins") ;
        set_long("A pile of " + coin_num + " " + coin_type + " coins.") ;
    }
}

void set_up(string type, int num) {
    mixed *config = mud_config("CURRENCY") ;
    int conversion ;

    if(nullp(type) || nullp(num) || num < 1) {
        return set_up(config[0][0], 1) ;
    }

    foreach(mixed *c in config) {
        if(c[0] == type) {
            conversion = c[1] ;
            break ;
        }
    }

    if(nullp(conversion)) {
        remove() ;
        error("Invalid coin type: " + type) ;
    }

    coin_type = type ;
    coin_num = num ;
    value = num * conversion ;

    update_descriptions() ;

    set_mass(num) ;
}

/**
 * 
 * @param {STD_CONTAINER} dest 
 */
int move(mixed dest) {
    object env = environment() ;
    int result ;

    if(!objectp(dest))
        dest = load_object((string)dest) ;

    if(mud_config("USE_MASS")) {
        if(!dest->ignore_capacity()) {
            if(!dest->can_hold_object(this_object())) {
                return MOVE_TOO_HEAVY ;
            }
        }
    }

    if(living(dest)) {
        // We can already move, so let's just add the coins to the living object
        // and then remove ourselves.
        dest->adjust_wealth(coin_type, coin_num) ;
        remove() ;
        return MOVE_OK ;
    } else {
        object *coin_obs = present_clones(base_name(), dest) ;

        coin_obs = filter(coin_obs, (: $1->query_coin_type() == coin_type :)) ;

        if(sizeof(coin_obs) > 0) {
            coin_obs[0]->adjust_coin_num(coin_num) ;
            remove() ;
            return MOVE_OK ;
        }

        result = ::move(dest) ;
    }
}

int adjust_coin_num(int num) {
    object env = environment() ;
    int new_sum  = coin_num + num ;

    if(new_sum < 0)
        return 0 ;

    if(!adjust_mass(num))
        return 0 ;

    if(new_sum == 0) {
        remove() ;
        return 1 ;
    }

    coin_num = new_sum ;
    value = CURRENCY_D->convert_currency(
        new_sum,
        coin_type,
        mud_config("CURRENCY")[0][0]
    ) ;

    update_descriptions() ;

    return 1 ;
}

string query_coin_type() { return coin_type ; }
int query_coin_num() { return coin_num ; }
mixed *query_value() { return ({ coin_num, coin_type }) ; }
int query_base_value() { return value ; }

int is_coin() { return 1 ; }
