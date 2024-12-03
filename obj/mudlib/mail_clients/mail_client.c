/*

 mail_client.c

 Parthenon@LPUniversity
 July 24th, 2006
 The mail client that allows access to all mailing functions

 Last edited on August 22nd, 2006 by Parthenon

*/

/* QC by Tacitus on July 13th, 2006 */

inherit STD_ITEM ;

#define MAIL_DIR "/data/users/"
#define MAX_SUBJECT_SIZE 65
#define MAX_MESSAGES_TO_DISPLAY 20

nosave string owner, save_path, *recipients, subject, *cc, date, body, current_file ;
nosave int max_in_msgs, max_out_msgs, did_write, greply, gforward, gflag ;
mapping inbox ;
mapping outbox ;
int curr_in_msg, curr_out_msg, in_outbox ;
int in_start_index, in_end_index, out_start_index, out_end_index ;
/** @type {OBJ_SECURITY_EDITOR} */
object s_editor ;

protected void main_menu() ;
int has_new_mail() ;
protected varargs void do_mail(string arg, int flag, int forward) ;
protected varargs void do_read_mail() ;
protected void resync_mailbox() ;
protected void read_message(int num) ;
protected varargs void do_help(int flag) ;
protected void prompt() ;
void get_subject(string arg, int flag, int forward) ;
protected varargs void do_save(int flag, int num1, int num2) ;
protected void save_mailbox() ;
protected mixed save_message(string path, mapping message) ;
void done_reading() ;
void client(object tp) ;

void setup() {
    inbox = ([ ]) ;
    outbox = ([ ]) ;
    s_editor = new(OBJ_SECURITY_EDITOR) ;
    add_init((: client :)) ;
}//END create

/**
 * 
 * @param {STD_PLAYER} tp 
 */
void client(object tp) {
    int i ;

    owner = tp->query_real_name() ;

    if(!userp(find_player(owner))) {
        remove() ;
        return ;
    }

    if(!owner) {
        remove() ;
        return ;
    }

    save_path = MAIL_D->get_mail_box_file(owner) ;

    resync_mailbox() ;

    for(i = 1; i <= max_in_msgs; i++) {
        if(inbox[i]["READ"] != 1) {
            curr_in_msg = i ;
            in_start_index = i ;
            break ;
        }
    }

    if(!curr_in_msg && max_in_msgs) curr_in_msg = max_in_msgs ;
    if(!curr_in_msg) curr_in_msg = 1 ;

    for(i = 1; i <= max_out_msgs; i++) {
        if(outbox[i]["READ"] != 1) {
            curr_out_msg = i ;
            out_start_index = i ;
            break ;
        }
    }

    if(!curr_out_msg && max_out_msgs) curr_out_msg = max_out_msgs ;
    if(!curr_out_msg) curr_out_msg = 1 ;
    if(!in_start_index) in_start_index = 1 ;
    if(!out_start_index) out_start_index = 1 ;

    save_mailbox() ;

    return ;
}//END init

int start_mail(string arg) {
    if(file_name(previous_object()) != "/cmds/wiz/mail") {
        remove() ;
        error("\nError [mail]: You must use the mail command to access mail\n") ;
    }

    if(!arg || arg == "") main_menu() ;
    else do_mail(arg, 1) ;

    return 1 ;
}//END start_mail

protected void main_menu() {
    write("\n\t" + mud_name() + " Mailer\n\n") ;
    write(" 1: Read mail" + (has_new_mail() ? " (unread mail)\n" : "\n")) ;
    write(" 2: Send new mail\n") ;
    write(" 3: Exit mailer\n") ;
    input_to("i_MainMenu") ;
}

protected varargs void i_MainMenu(string arg, int flag, int forward) {
    if(!arg || arg == "") {
        main_menu() ;
        return ;
    }

    switch(arg) {
        case "1": {
                write("\n\t" + mud_name() + " Mailer\n\n") ;
                write(" 1: Inbox" + (has_new_mail() ? " (unread mail) \n" : "\n")) ;
                write(" 2: Outbox\n") ;
                write(" 3: Main menu\n") ;
                input_to("i_InboxOutbox") ;
                return ;
            }
        case "2": {
                do_mail(0, flag, forward) ;
                return ;
            }

            case "q" :
            case "quit" :
            case "3": {
                    write("\nExiting " + mud_name() + " Mailer\n") ;
                    save_mailbox() ;
                    remove() ;
                    return ;
                }

            default: {
                    write("\nError [mail]: Invalid option\n") ;
                    main_menu() ;
                    return ;
                }
    }

    return ;
}//END i_MainMenu

protected void i_InboxOutbox(string arg) {
    if(!arg || arg == "") {
        i_MainMenu("1") ;
        return ;
    }

    switch(arg) {
        case "1": {
                in_outbox = 0 ;
                do_read_mail() ;
                return ;
            }

        case "2": {
                in_outbox = 1 ;
                do_read_mail() ;
                return ;
            }

        case "3": {
                main_menu() ;
                return ;
            }

        default: {
                write("\nError [mail]: Invalid option\n") ;
                i_MainMenu("1") ;
                return ;
            }
    }
}//End i_InboxOutbox

protected varargs void do_read_mail() {
    int *msg_keys ;
    int i ;
    string *tmp_recipients ;

    resync_mailbox() ;

    if(in_outbox) {
        msg_keys = keys(outbox) ;

        if(!msg_keys || !sizeof(msg_keys)) {
            write("\nYou have no messages in your outbox.\n") ;
            i_MainMenu("1") ;
            return ;
        }

        if(sizeof(outbox) <= MAX_MESSAGES_TO_DISPLAY) {
            out_start_index = 1 ;
            out_end_index = sizeof(outbox) ;
        } else {
            out_end_index = out_start_index + MAX_MESSAGES_TO_DISPLAY - 1 ;

            if(out_end_index > sizeof(outbox)) {
                out_start_index = sizeof(outbox) - MAX_MESSAGES_TO_DISPLAY + 1 ;
                out_end_index = sizeof(outbox) ;
            }
        }

        printf("%5s%4s%4s%-20s  %-30s %s\n", "Curr ", "New ", "Num ", "To", "Date", "Subject") ;

        write("_______________________________________________________________________________\n\n") ;

        for(i = out_start_index; i <= out_end_index; i++) {
                tmp_recipients = implode(outbox[i]["TO"], ", ") ;

                printf("   %s %-4s%-4d%-20s  %-30s %s\n",
                    i == curr_out_msg ? ">" : " ",
                    outbox[i]["READ"] ? " " : "*",
                    i,
                    (sizeof(tmp_recipients) > 19 ? tmp_recipients[0..19] : tmp_recipients),
                    outbox[i]["DATE"],
                    outbox[i]["SUBJECT"]
                ) ;
        }

        write("\n Displaying " + out_start_index + " to " + out_end_index + " of " + sizeof(outbox) + ".\n") ;
    } else {
        msg_keys = keys(inbox) ;

        if(!msg_keys || !sizeof(msg_keys)) {
            write("\nYou have no messages in your inbox.\n") ;
            i_MainMenu("1") ;
            return ;
        }

        if(sizeof(inbox) <= MAX_MESSAGES_TO_DISPLAY) {
            in_start_index = 1 ;
            in_end_index = sizeof(inbox) ;
        } else {
            in_end_index = in_start_index + MAX_MESSAGES_TO_DISPLAY - 1 ;

            if(in_end_index > sizeof(inbox)) {
                in_start_index = sizeof(inbox) - MAX_MESSAGES_TO_DISPLAY + 1 ;
                in_end_index = sizeof(inbox) ;
            }
        }

        printf("%5s%4s%4s%-20s  %-30s %s\n", "Curr ", "New ", "Num ", "From", "Date", "Subject") ;

        write("_______________________________________________________________________________\n\n") ;

        for(i = in_start_index; i <= in_end_index; i++) {
            printf("   %s %-4s%-4d%-20s  %-30s %s\n",
                i == curr_in_msg ? ">" : " ",
                inbox[i]["READ"] ? " " : "*",
                i,
                inbox[i]["FROM"],
                inbox[i]["DATE"],
                inbox[i]["SUBJECT"]
            ) ;
        }

        write("\n Displaying " + in_start_index + " to " + in_end_index + " of " + sizeof(inbox) + ".\n") ;
    }

    write("\n\n") ;
    prompt() ;
}//END do_read_mail

protected void prompt() {
    write("Enter selection ('h' for help): ") ;
    input_to("parse_mailcmd") ;
}

protected varargs void parse_mailcmd(string arg) {
    int max_msgs, curr_msg, i ;
    int num1 = to_int(arg), num2 ;
    mixed err ;

    resync_mailbox() ;

    if(!arg || arg == "") {
        do_read_mail() ;
        return ;
    }

    arg = lower_case(arg) ;

    if(num1 < 0 || (in_outbox && num1 > sizeof(outbox)) || (!in_outbox && num1 > sizeof(inbox))) {
        write("\nError [mail]: Invalid message number\n\n") ;
        do_read_mail() ;
        return ;
    }

    if(num1) {
        if(in_outbox) {
            if((num1 > out_end_index) || (num1 < out_start_index))
                out_start_index = num1 ;
        } else {
            if((num1 > in_end_index) || (num1 < in_start_index))
                in_start_index = num1 ;
        }

        read_message(num1) ;
        return ;
    }

    if(in_outbox) {
        max_msgs = max_out_msgs ;
        curr_msg = curr_out_msg ;
    } else {
        max_msgs = max_in_msgs ;
        curr_msg = curr_in_msg ;
    }

    if(arg == "d all" || arg == "del all") {
        arg = "d" ;
        num1 = 1 ;

        if(in_outbox) num2 = sizeof(outbox) ;
        else num2 = sizeof(inbox) ;
    }

    sscanf(arg, "%s %d,%*(*)%d", arg, num1, num2) ;

    if(!num1) num1 = curr_msg ;

    switch(arg) {
        case "u": {
                if(in_outbox) {
                    if(num2) {
                        for(i = num1; i <= num2; i++)
                            outbox[i]["READ"] = 0 ;
                        curr_out_msg = num2 ;
                    }
                    else outbox[num1]["READ"] = 0 ;
                } else {
                    if(num2) {
                        for(i = num1; i <= num2; i++)
                            inbox[i]["READ"] = 0 ;
                        curr_in_msg = num2 ;
                    }
                    else inbox[num1]["READ"] = 0 ;
                }

                write("\nSuccess [mail]: Message " + num1 + " marked as unread\n\n") ;
                save_mailbox() ;
                prompt() ;
                return ;
            }

        case "d":
        case "del": {
                err = MAIL_D->delete_message(owner, num1, num2, in_outbox, curr_msg) ;

                if(stringp(err)) {
                    write("\nError [mail]: " + capitalize(err) + "\n\n") ;
                    resync_mailbox() ;
                    prompt() ;
                    return ;
                } else {
                    write("Success [mail]: Message successfully deleted\n\n") ;
                    resync_mailbox() ;
                    prompt() ;
                    return ;
                }
            }

        case "n":
        case "next": {
                if((curr_msg+1) > max_msgs) {
                    write("\nError [mail]: You have no more messages\n\n") ;
                    do_read_mail() ;
                    return ;
                }
                else read_message((curr_msg + 1)) ;

                return ;
            }

        case "p":
        case "prev":
        case "previous": {
                if((curr_msg - 1) < 1) {
                    write("\nError [mail]: There are no previous messages\n\n") ;
                    do_read_mail() ;
                    return ;
                }
                else read_message((curr_msg - 1)) ;
                return ;
            }

        case "l":
        case "list": {
                do_read_mail() ;
                return ;
            }

        case "m":
        case "main": {
                i_MainMenu("1") ;
                return ;
            }

        case "r":
        case "reply": {
                in_outbox ? curr_out_msg = num1 : curr_in_msg = num1 ;
                greply = num1 ;
                write("\nCC: ") ;
                input_to("get_cc", 0, 0) ;
                return ;
            }

        case "f":
        case "forward": {
                if(in_outbox) curr_out_msg = num1 ;
                else curr_in_msg = num1 ;

                write("\nEnter the recipient/s of your mail.\n" +
                        "Seperate multiple names with a comma: ") ;
                input_to("get_recipient", 0, num1) ;
                return ;
            }

        case "s":
        case "save": {
                do_save(num1, num2) ;
                return ;
            }

        case "q":
        case "quit": {
                write("\nExiting " + mud_name() + " Mailer\n") ;
                save_mailbox() ;
                remove() ;
                return ;
            }

        case "h":
        case "help":
        case "?": {
                do_help() ;
                return ;
            }

        default: {
                write("\nError [mail]: Invalid option\n\n") ;
                do_read_mail() ;
                return ;
            }
    }
}//END parse_mailcmd

protected varargs void do_save(int num1, int num2) {
    write("Please enter destination: ") ;
    input_to("i_SaveDir", 0, num1, num2) ;
}//END do_save

protected varargs void i_SaveDir(string arg, int num1, int num2) {
    string path ;
    mapping message = ([]) ;
    mixed err ;
    int i ;

    if(!arg || arg == "") {
        write("\nError [mail]: No save file specified\n\n") ;
        do_read_mail() ;
        return ;
    }

    if(arg[<1] == '/') {
        write("\nError [mail]: You must specify a full path including filename\n\n") ;
        do_read_mail() ;
        return ;
    }

    else if(file_size(arg) == -2) {
        write("\nError [mail]: You cannot save there, that is a directory name.\n\n") ;
        do_read_mail() ;
        return ;
    }

    if(arg[0] == '/') path = arg ;
    else path = find_player(owner)->query_env("cwd") + "/" + arg ;

    if(file_exists(path)) {
        write("\nThat file already exists... would you like to overwrite it: ") ;
        input_to("i_Overwrite", 0, path, num1, num2) ;
        return ;
    }

    if(in_outbox) {
        if(num2) {
            for(i = 0; i <= (num2-num1+1); i++) {
                message[i] = outbox[num1+i] ;
                curr_out_msg = num2 ;
            }
        } else {
            message[0] = outbox[num1] ;
            curr_out_msg = num1 ;
        }
    } else {
        if(num2) {
            for(i = 0; i <= (num2-num1+1); i++) {
                message[i] = inbox[num1+i] ;
                curr_in_msg = num2 ;
            }
        }

        else {
            message[0] = inbox[num1] ;
            curr_in_msg = num1 ;
        }
    }

    err = save_message(path, message) ;

    if(stringp(err)) {
        write("\nError [mail]: " + capitalize(err) + "\n\n") ;
        do_read_mail() ;
        return ;
    } else {
        write("\nSuccess [mail]: Message successfully saved to " + path + "\n\n") ;
        do_read_mail() ;
        return ;
    }
}//END i_SaveDir

protected varargs void i_Overwrite(string arg, string path, int num1, int num2) {
    mixed err ;
    mapping message = ([]) ;
    int i ;

    if(member_array(lower_case(arg), ({ "y", "n", "yes", "no" })) == -1) {
        write("Please enter yes or no: ") ;
        input_to("i_Overwrite", 0, path, num1, num2) ;
        return ;
    }

    if(in_outbox) {
        if(num2) {
            for(i = 0; i <= (num2-num1+1); i++) {
                message[i] = outbox[num1+i] ;
                curr_out_msg = num2 ;
            }
        } else {
            message[0] = outbox[num1] ;
            curr_out_msg = num1 ;
        }
    } else {
        if(num2) {
            for(i = 0; i <= (num2-num1+1); i++) {
                message[i] = inbox[num1+i] ;
                curr_in_msg = num2 ;
            }
        } else {
            message[0] = inbox[num1] ;
            curr_in_msg = num1 ;
        }
    }

    if(lower_case(arg) == "y" || lower_case(arg) == "n") {
        err = save_message(path, message) ;

        if(stringp(err)) {
            write("\nError [mail]: " + capitalize(err) + "\n\n") ;
            do_read_mail() ;
            return ;
        } else {
            write("\nSuccess [mail]: Message successfully saved to " + path + "\n\n") ;
            do_read_mail() ;
            return ;
        }
    } else {
        write("\nMessage save was cancelled\n") ;
        do_read_mail() ;
        return ;
    }
}//END i_Overwrite

protected void read_message(int num) {
    string ret = "" ;

    if(in_outbox) {
        ret += "\nFROM:    " + outbox[num]["FROM"] + "\n" ;
        ret += "TO:      " + implode(outbox[num]["TO"], ", ") + "\n" ;
        ret += "SUBJECT: " + outbox[num]["SUBJECT"] + "\n" ;
        ret += "DATE:    " + outbox[num]["DATE"] + "\n" ;
        ret += "CC:      " + implode(outbox[num]["CC"], ", ") + "\n" ;
        ret += "_____________________________________________________________________\n" ;
        ret += outbox[num]["BODY"] + "\n" ;
        outbox[num]["READ"] = 1 ;
        curr_out_msg = num ;
        save_mailbox() ;
        resync_mailbox() ;
        as_player(environment())->page(ret, assemble_call_back("done_reading"), 0) ;
        return ;
    } else {
        ret += "\nFROM:    " + inbox[num]["FROM"] + "\n" ;
        ret += "TO:      " + implode(inbox[num]["TO"], ", ") + "\n" ;
        ret += "SUBJECT: " + inbox[num]["SUBJECT"] + "\n" ;
        ret += "DATE:    " + inbox[num]["DATE"] + "\n" ;
        ret += "CC:      " + implode(inbox[num]["CC"], ", ") + "\n" ;
        ret += "____________________________________________________________\n" ;
        ret += inbox[num]["BODY"] + "\n" ;
        inbox[num]["READ"] = 1 ;
        curr_in_msg = num ;
        save_mailbox() ;
        resync_mailbox() ;
        as_player(environment())->page(ret, assemble_call_back("done_reading"), 0) ;
        return ;
    }
}

void done_reading() {
    write("\n\n") ;
    prompt() ;
    return ;
}

protected varargs void do_help() {
    write("\n\t" + mud_name() + " Mailer Help\n\n") ;
    printf("%-23s- %s\n", "[<num>]", "Read the current message or message num.") ;
    printf("%-23s- %s\n", "(r)eply [<num>]", "Reply to the current message or message num.") ;
    printf("%-23s- %s\n", "(n)ext", "Read the next message.") ;
    printf("%-23s- %s\n", "(p)rev", "Read the previous message.") ;
    printf("%-23s- %s\n", "(f)orward [<num>]", "Forward the current message or message num.") ;
    printf("%-23s- %s\n", "(d)el [<num>,<num>]", "Delete the current message or message num.") ;
    printf("%25' 's%s\n%25' 's%s\n", "", "You may also delete a range of messages by seperating", "", "two numbers with a comma.") ;
    printf("%25' 's%s\n%25' 's%s\n", "", "If you wish to delete all of your messages then use", "", "'d all' or 'del all'.") ;
    printf("%-23s- %s\n", "(s)ave [<num>,<num>]", "Save current message or message num to file.") ;
    printf("%25' 's%s\n%25' 's%s\n", "", "You may also save a range of messages to a file by seperating", "", "two numbers with a comma.") ;
    printf("%-23s- %s\n", "(u)nread [<num>,<num>]", "Mark current message or message num as unread.") ;
    printf("%25' 's%s\n%25' 's%s\n", "", "You may also mark as unread a range of messages by seperating", "", "two numbers with a comma.") ;
    printf("%-23s- %s\n", "(l)ist", "Relist your messages.") ;
    printf("%-23s- %s\n", "(m)ain", "Back to the menu.") ;
    printf("%-23s- %s\n", "(q)uit", "Exit the mailer.") ;
    write("\n\n") ;
    prompt() ;
}

protected varargs void do_mail(string arg, int flag, int forward) {
    int i ;
    string *recipients_tmp ;

    if(!arg || arg == "") {
        write("\nEnter the recipient/s of your mail.\nSeperate multiple names with a comma: ") ;
        input_to("get_recipient", 0, flag, forward) ;
        return ;
    }

    recipients = ({}) ;
    recipients = explode(replace_string(arg, " ", ""), ",") ;
    recipients_tmp = recipients ;

    for(i = 0; i < sizeof(recipients); i++) {
        if(!ofile_exists(user_data_file(recipients[i]))) {
            if(recipients[i][0] == '(' && recipients[i][<1] == ')') {
                if(member_array(recipients[i][1..<2], s_editor->list_groups()) == -1)
                    recipients_tmp -= ({ recipients[i] }) ;
            }
            else recipients_tmp -= ({ recipients[i] }) ;
        }
    }

    recipients = recipients_tmp ;

    if(!recipients || sizeof(recipients) == 0) {
        write("\nError [mail]: No valid recipients\n") ;
        return ;
    }

    write("\nPlease enter the subject line: ") ;
    input_to("get_subject", 0, flag, forward) ;
}//END do_mail

protected void get_recipient(string arg, int flag, int forward) {
    int i ;
    string *recipients_tmp ;

    if(!arg || arg == "") {
        write("\nError [mail]: No recipient/s specified\n") ;
        main_menu() ;
        return ;
    }

    recipients = ({}) ;
    recipients = explode(replace_string(arg, " ", ""), ",") ;
    recipients_tmp = recipients ;

    for(i = 0; i < sizeof(recipients); i++) {
        if(!ofile_exists(user_data_file(recipients[i]))) {
            if(recipients[i][0] == '(' && recipients[i][<1] == ')') {
                if(member_array(recipients[i][1..<2], s_editor->list_groups()) == -1)
                    recipients_tmp -= ({ recipients[i] }) ;
            }
            else recipients_tmp -= ({ recipients[i] }) ;
        }
    }

    recipients = recipients_tmp ;

    if(!recipients || sizeof(recipients) == 0) {
        write("\nError [mail]: No valid recipients\n") ;
        main_menu() ;
        return ;
    }

    if(forward) get_subject(0, flag, forward) ;
    else {
        write("\nPlease enter the subject line: ") ;
        input_to("get_subject", 0, flag, forward) ;
    }
}//END get_recipient

void get_subject(string arg, int flag, int forward) {
    if(!arg || arg == "") {
        if(forward && flag)subject = "FWD: " + outbox[forward]["SUBJECT"] ;
        else if(forward)subject = "FWD: " + inbox[forward]["SUBJECT"] ;
        else subject = "<no subject>" ;
    } else {
        if(strlen(arg) > MAX_SUBJECT_SIZE) {
            write("\nSubject size must not exceed " + MAX_SUBJECT_SIZE+ " characters\n\nSubject: ") ;
            input_to("get_subject", 0, flag) ;
            return ;
        }
        else subject = arg ;
    }

    if(!subject) subject = "<no subject>" ;

    write("\nCC: ") ;
    input_to("get_cc", 0, flag, forward) ;
}//END get_subject

protected varargs void get_cc(string arg, int flag, int forward) {
    int i ;
    string *cc_tmp ;

    if(!arg || arg == "") {
        cc = ({ }) ;
    } else {
        cc = ({}) ;
        cc = explode(replace_string(arg, " ", ""), ",") ;
        cc_tmp = cc ;

        for(i = 0; i < sizeof(cc); i++) {
            if(!ofile_exists(user_data_file(cc[i]))) {
                if(cc[i][0] == '(' && cc[i][<1] == ')') {
                    if(member_array(cc[i][1..<2], s_editor->list_groups()) == -1)
                        cc_tmp -= ({ cc[i] }) ;
                }
                else {
                    cc_tmp -= ({ cc[i] }) ;
                }
            }
        }
    }

    cc = cc_tmp ;

    if(!cc || sizeof(cc) == 0)cc = ({ }) ;

    date = "" ;
    date = ctime(time()) ;

    if(greply) {
        if(in_outbox) {
            write("\nFROM:    " + capitalize(owner) + "\n") ;
            write("TO:      " + outbox[greply]["FROM"] + "\n") ;
            write("SUBJECT: " + "RE: " + outbox[greply]["SUBJECT"] + "\n") ;
            write("DATE:    " + date + "\n") ;
            write("CC:      " + implode(cc, ", ") + "\n") ;
            write("____________________________________________________________\n") ;
        } else {
            write("\nFROM:    " + capitalize(owner) + "\n") ;
            write("TO:      " + inbox[greply]["FROM"] + "\n") ;
            write("SUBJECT: " + "RE: " + inbox[greply]["SUBJECT"] + "\n") ;
            write("DATE:    " + date + "\n") ;
            write("CC:      " + implode(cc, ", ") + "\n") ;
            write("____________________________________________________________\n") ;
        }
    } else {
        write("\nFROM:    " + capitalize(owner) + "\n") ;
        write("TO:      " + implode(recipients, ", ") + "\n") ;
        write("SUBJECT: " + subject + "\n") ;
        write("DATE:    " + date + "\n") ;
        write("CC:      " + implode(cc, ", ") + "\n") ;
        write("____________________________________________________________\n") ;
    }

    current_file = "/tmp/" + random(9999999) + "." + this_body()->query_real_name() ;
        while(file_exists(current_file))
            current_file = "/tmp/" + random(9999999) + "." + this_body()->query_real_name() ;

        gflag = flag ;
        gforward = forward ;

    write_file(current_file, "") ;
    if(!devp(this_body())) ed(current_file, "callback_write", "callback_exit", 1) ;
    else ed(current_file, "callback_write", "callback_exit", 0) ;
}//END get_cc

int callback_write(string fname, int flag) {
    if(!master()->valid_read(fname, this_object(), "callback_exit")) return 0 ;
    current_file = fname ;
    did_write = 1 ;
    return 1 ;
}

void callback_exit() {
    string err ;
    mapping newmsg = ([]) ;
    string body = "" + read_file(current_file) ;

    if(greply) {
        body += "\n\n> ----------ORIGINAL MESSAGE----------\n" ;
        body += "> FROM: " + (gflag ? outbox[greply]["FROM"] + "\n" : inbox[greply]["FROM"] + "\n") ;
        body += "> TO: " + (gflag ? implode(outbox[greply]["TO"], ", ") + "\n" : implode(inbox[greply]["TO"], ", ") + "\n") ;
        body += "> SUBJECT: " + (gflag ? outbox[greply]["SUBJECT"] + "\n" : inbox[greply]["SUBJECT"] + "\n") ;
        body += "> DATE: " + (gflag ? outbox[greply]["DATE"] + "\n" : inbox[greply]["DATE"] + "\n") ;
        body += "> CC: " + (gflag ? implode(outbox[greply]["CC"], ", ") + "\n" : implode(inbox[greply]["CC"], ", ") + "\n> ") ;
        body += "\n> " + implode(explode((gflag ? outbox[greply]["BODY"] : inbox[greply]["BODY"]), "\n"), "\n> ") ;
    } else if(gforward) {
        body += "\n\n> ----------FORWARDED MESSAGE----------\n" ;
        body += "> FROM: " + (gflag ? outbox[gforward]["FROM"] + "\n" : inbox[gforward]["FROM"] + "\n") ;
        body += "> TO: " + (gflag ? implode(outbox[gforward]["TO"], ", ") + "\n" : implode(inbox[gforward]["TO"], ", ") + "\n") ;
        body += "> SUBJECT: " + (gflag ? outbox[gforward]["SUBJECT"] + "\n" : inbox[gforward]["SUBJECT"] + "\n") ;
        body += "> DATE: " + (gflag ? outbox[gforward]["DATE"] + "\n" : inbox[gforward]["DATE"] + "\n") ;
        body += "> CC: " + (gflag ? implode(outbox[gforward]["CC"], ", ") + "\n" : implode(inbox[gforward]["CC"], ", ") + "\n> ") ;
        body += "\n> " + implode(explode((gflag ? outbox[gforward]["BODY"] : inbox[gforward]["BODY"]), "\n"), "\n> ") ;
    }

    if(!body || !did_write) {
        write("\nSend new mail aborted\n") ;
        rm(current_file) ;

        if(gflag) {
            remove() ;
            return ;
        } else {
            main_menu() ;
            return ;
        }
    }

    if(greply) {
        if(gflag) {
            newmsg["TO"] = ({ outbox[greply]["FROM"] }) ;
            newmsg["SUBJECT"] = "RE: " + outbox[greply]["SUBJECT"] ;
        } else {
            newmsg["TO"] = ({ inbox[greply]["FROM"] }) ;
            newmsg["SUBJECT"] = "RE: " + inbox[greply]["SUBJECT"] ;
        }
    } else {
        newmsg["TO"] = recipients ;
        newmsg["SUBJECT"] = subject ;
    }

    newmsg["FROM"] = capitalize(owner) ;
    newmsg["CC"] = cc ;
    newmsg["DATE"] = date ;
    newmsg["BODY"] = body ;

    err = MAIL_D->send_message(newmsg, owner, curr_in_msg, curr_out_msg) ;

    if(stringp(err)) write("\nError [mail]: " + capitalize(err) + "\n") ;
    else {
        write("\nSuccess [mail]: Mail successfully sent\n") ;
        resync_mailbox() ;
    }

    rm(current_file) ;
    current_file = "" ;

    if(gflag) remove() ;
    else main_menu() ;
}

int has_new_mail() {
    int i ;

    resync_mailbox() ;

    for(i = 1; i <= max_in_msgs; i++)
    if(inbox[i]["READ"] != 1) return 1 ;

    return 0 ;
}//END has_new_mail

protected void resync_mailbox() {
    mapping vars = ([]) ;

    if(file_exists(save_path)) {
        inbox = ([]) ;
        outbox = ([]) ;
        vars = MAIL_D->restore(owner) ;
        inbox = vars["inbox"] ;
        outbox = vars["outbox"] ;
        curr_in_msg = vars["curr_in_msg"] ;
        curr_out_msg = vars["curr_out_msg"] ;
        in_start_index = vars["in_start_index"] ;
        in_end_index = vars["in_end_index"] ;
        out_start_index = vars["out_start_index"] ;
        out_end_index = vars["out_end_index"] ;
        max_in_msgs = sizeof(inbox) ;
        max_out_msgs = sizeof(outbox) ;

        if(curr_in_msg > sizeof(inbox)) curr_in_msg = sizeof(inbox) ;
        if(curr_out_msg > sizeof(outbox)) curr_out_msg = sizeof(outbox) ;
    }
}//END resync_mailbox

protected void save_mailbox() {
    MAIL_D->save(owner, inbox, outbox, ({ curr_in_msg, curr_out_msg, in_start_index, in_end_index, out_start_index, out_end_index })) ;
}

protected mixed save_message(string path, mapping message) {
    int i ;
    string text = "" ;

    if(sizeof(message) == 1){
        text += "FROM:    " + message[0]["FROM"] + "\n" ;
        text += "TO:      " + implode(message[0]["TO"], ", ") + "\n" ;
        text += "SUBJECT: " + message[0]["SUBJECT"] + "\n" ;
        text += "DATE:    " + message[0]["DATE"] + "\n" ;
        text += "CC:      " + implode(message[0]["CC"], ", ") + "\n" ;
        text += "____________________________________________________________\n" ;
        text += message[0]["BODY"] + "\n" ;
    } else {
        for(i = 0; i < sizeof(message); i++) {
            text += "FROM:    " + message[i]["FROM"] + "\n" ;
            text += "TO:      " + implode(message[i]["TO"], ", ") + "\n" ;
            text += "SUBJECT: " + message[i]["SUBJECT"] + "\n" ;
            text += "DATE:    " + message[i]["DATE"] + "\n" ;
            text += "CC:      " + implode(message[i]["CC"], ", ") + "\n" ;
            text += "____________________________________________________________\n" ;
            text += message[i]["BODY"] + "\n\n" ;
            text += sprintf("%79'='s\n%79'='s\n\n\n\n", "", "") ;
        }
    }

    if(!master()->valid_write(path, this_body(), "save_message"))
    return "Could not write file to " + path ;

    write_file(path, text, 1) ;

    return 1 ;
}

int remove() {
    if(objectp(s_editor))
        s_editor->remove() ;
    return ::remove() ;
}
