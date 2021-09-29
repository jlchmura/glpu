 /* tell.c

 Tricky @ LPUniversity
 18-AUG-2006
 Intermud3 tell service

 */

varargs void send_error(mixed *packet, string err_code, string err_msg, int send_packet);

void send_tell(object o_user, mixed d_user, string d_mud, string msg)
{
    string user;

    if(objectp(d_user)) user = lower_case(d_user->query_name());
    else
    if(stringp(d_user)) user = lower_case(d_user);
    else return;

    send_packet( ({
      "tell",
      5,
      mud_name(),
      lower_case(query_privs(o_user)),
      d_mud,
      user,
      capitalize(query_privs(o_user)),
      msg
    }) );
}

void rec_tell(mixed *packet)
{
    object o_user;

    if(!packet[D_USER] || !(o_user = find_player(packet[D_USER])))
    {
        send_error(packet, "unk-user", packet[D_USER] + "@" + packet[D_MUD] + " was not found!", 1);
        return;
    }

    tell_object(o_user, sprintf("%s@%s tells you: %s\n", "%^CYAN%^" + packet[6], packet[O_MUD], "%^RESET%^" + packet[7]));
    o_user->set("reply", packet[6] + "@" + packet[O_MUD]);

    if(o_user->query_env("away") != 0)
        send_tell(o_user, packet[O_USER], packet[O_MUD], o_user->query_env("away"));

}
