/* callouts.c

 Tacitus @ LPUniversity
 05-NOV-05
 Wizard cmd

*/

inherit STD_CMD ;

#define BORDER "o------------------------------------------------------------------------------o\n"

mixed main(object tp, object room, string arg) {
     int i;
     mixed *data = call_out_info();

     if(sizeof(data) < 1) return(notify_fail("Error [callouts]: There are currently no callouts in progress.\n"));
     tell(tp, sprintf("%10s %35s %28s\n%s", "Object", "Function", "Delay", BORDER));
     for(i = 0; i < sizeof(data); i++)
          tell(tp, sprintf("%-40s %-30s %-5d\n", file_name(data[i][0]), data[i][1], to_int(data[i][2])));
     tell(tp, BORDER) ;
     tell(tp, sprintf("%48s", "Total Callouts: " + sizeof(data) + "\n")) ;
     return 1;
}

string help(object caller) {
     return(" SYNTAX: callouts\n\n"
     "This comand will return information about currently active callouts.\n"
     "When you call this command, it will inform you of each object with\n"
     "an active callout, what function will be called, and how many\n"
     "seconds (the delay) until the before mentioned function will be\n"
     "called. This is useful debugging tool for objects that make use\n"
     "of callouts.\n");
}
