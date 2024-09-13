/**
 * @file /cmds/wiz/env.c
 * @description Manage environment settings for a wizard.
 *
 * @created 2024-08-17 - Gesslar
 * @last_modified 2024-08-17 - Gesslar
 *
 * @history
 * 2024-08-17 - Gesslar - Created
 */

inherit STD_CMD ;

void setup() {
    usage_text =
    "env - view your current settings\n"
    "env <setting> - delete a setting\n"
    "env <setting> <value> - set a setting\n"
    "env <setting>_colour prompt - prompt for a colour\n"
    ;

    help_text =
"This command allows you, the user, to setup, modify, or delete settings "
"for your player object.\n\n"
"To set a setting, you would type 'env setting value'. If you omit "
"the second argument and a setting with a name matching the first argument "
"exists, then that setting will be deleted.\n\n"
"If the preference name ends with '_colour', and the value is 'prompt', then "
"you will be prompted to select a colour from the available colours.\n\n"
"See also: preferences, colour\n\n"
"View {{ul1}}help settings{{ul0}} for available settings.";
}

void prompt_colour_result(string input, object tp, string variable) ;

mixed main(object tp, string str) {
    string var_name, var_value, *keys;
    mapping data;
    int i;

    if(!str) {
        data = tp->list_env();
        if(!mapp(data))
            return _info("No settings currently set.");
        else {
            if(!sizeof(data))
                return _info("No settings currently set.");

            keys = keys(data);
            keys = sort_array(keys, 1);
            tell(tp, "Current settings:\n\n") ;

            for(i = 0; i < sizeof(keys); i ++)
                tell(tp,
                        sprintf("%-20s : %-20s\n", keys[i], data[keys[i]]),
                        NO_COLOUR
                ) ;

            return 1;
        }
    }

    sscanf(str, "%s %s", var_name, var_value);

    if(!var_name)
     var_name = str;

    if(strlen(var_name) > 20)
        return _error("Setting name too long.");

    if(pcre_match(var_name, "^[a-zA-Z0-9_]+_colour$") && var_value == "prompt") {
        prompt_colour(
            tp,
            assemble_call_back((: prompt_colour_result :), tp, var_name),
            "Select the colour you would like to set for '" + var_name + "':"
        );

        return 1;
    }

    tp->set_env(var_name, var_value);

    if(!var_value) {
        _ok(tp, "Value '" + var_name + "' cleared.");
        emit(SIG_USER_ENV_CHANGED, tp, var_name, "");
    } else {
        _ok(tp, "Value '" + var_name + "' set to '" + var_value + "'.");
        emit(SIG_USER_ENV_CHANGED, tp, var_name, var_value);
    }

    return 1;
}

void prompt_colour_result(string result, object tp, string variable) {
    switch(result) {
        case "cancel":
            _ok("You opt not to set this value at this time.") ;
            return;
        case "plain":
            tp->set_env(variable, "");
            _ok(tp, "Selected plain for %s.", variable);
            return;
        default: {
            string colour ;

            result = COLOUR_D->substitute_too_dark(result);
            colour = sprintf("{{0%s}}", result) ;
            tp->set_env(variable, result);
            _ok(tp, "You have selected %s%s{{res}} for `%s`.",
                colour,
                result,
                variable
            ) ;
            return;
        }
    }
}