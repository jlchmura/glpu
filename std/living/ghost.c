/**
 * @file /std/living/ghost.c
 * @description You're a ghost OooOoOOOOoOOoOooooOOo
 *
 * @created 2024-07-28 - Gesslar
 * @last_modified 2024-07-28 - Gesslar
 *
 * @history
 * 2024-07-28 - Gesslar - Created
 */

#include <logs.h>
#include <origin.h>
#include <gmcp_defines.h>

inherit STD_BODY ;

inherit M_GMCP ;

void mudlib_setup() {
    // if(origin() != ORIGIN_DRIVER && origin() != ORIGIN_LOCAL) return ;
    path = ({"/cmds/ghost/"}) ;
    set_log_level(0) ;
    if(clonep())
        slot(SIG_SYS_CRASH, "on_crash") ;
}

/* Connection functions */
void setup_body() {
    add_action("command_hook", "", 1) ;
    add_action("revive", "revive") ;
    set_living_name(query_real_name()) ;
    set_id(({query_real_name()})) ;
    set_heart_beat(mud_config("DEFAULT_HEART_RATE")) ;
    set_race("ghost") ;
    set_level(1.0) ;
    set_hp(1.0) ;
    set_sp(1.0) ;
    set_mp(1.0) ;
    set_env("move_out", "$N drifts away to the $D.") ;
    set_env("move_in", "$N drifts into the area.") ;
    enable_commands() ;
    if(!query_short()) set_short(query_name()) ;
    if(!query_pref("colour")) set_pref("colour", "off") ;
    if(!query_pref("prompt")) set_pref("prompt", ">") ;
    if(!query_pref("biff")) set_pref("biff", "off") ;
    update_regen_interval() ;
    init_vitals() ;
    set_log_prefix(sprintf("(%O)", this_object())) ;
}

int revive(string str) {
    /** @type {STD_PLAYER} */
    object body ;
    string name = query_privs() ;

    if(this_body() != this_object())
        return 0 ;

    body = BODY_D->create_body(name) ;

    exec(body, this_object()) ;

    body->setup_body() ;
    body->set_dead(0) ;
    body->set_hp(1.0) ;
    body->set_sp(1.0) ;
    body->set_mp(1.0) ;

    body->move(environment()) ;
    body->simple_action("$N $vhave revived.") ;

    if(body->gmcp_enabled())
        GMCP_D->init_gmcp(body) ;

    emit(SIG_PLAYER_REVIVED, body) ;

    remove() ;

    return 1 ;
}

void net_dead() {
    if(origin() != ORIGIN_DRIVER)
        return ; 

    if(environment())
        tell_all(environment(), query_name()+ " begins to fade.\n") ;

    add_extra_short("link_dead", "[fading]") ;
    log_file(LOG_LOGIN, query_real_name() + " went link-dead on " + ctime(time()) + "\n") ;

}

void reconnect() {
    tell(this_object(), "Success: Reconnected.\n") ;
    if(environment()) tell_room(environment(), query_name() + " brigtens.\n", this_body()) ;
    remove_extra_short("link_dead") ;
}

/* Body Object Functions */
void heart_beat() {
    if(userp()) {
        if(!interactive(this_object())) {
            if((time() - query_last_login()) > 3600) {
                if(environment())
                    tell_room(environment(), query_name() + " fades out of existance.\n") ;
                log_file(LOG_LOGIN, query_real_name() + " auto-quit after 1 hour of net-dead at " + ctime(time()) + ".\n") ;
                remove() ;
                return ;
            }
        } else {
            /* Prevent link-dead from idle */
            if(query_idle(this_object()) % 60 == 0 && query_idle(this_object()) > 300
                    && query_pref("keepalive") && query_pref("keepalive") != "off") {
                telnet_nop() ;
            }
        }
    }

    if(gmcp_enabled())
        GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, null) ;
}

int is_ghost() { return 1 ; }
