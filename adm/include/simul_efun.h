#ifndef __SIMUL_EFUN_H__
#define __SIMUL_EFUN_H__

// File: arrays
mixed *distinct_array(mixed *arr) ;
varargs mixed *remove_array_element(mixed *arr, int start, int end) ;
varargs mixed *splice(mixed *arr, int start, int deleteCount, mixed *itemsToAdd) ;
mixed *reverse_array(mixed *arr) ;
int uniform_array(string type, mixed *arr) ;
varargs mixed array_fill(mixed *arr, mixed value, int size, int start_index) ;
varargs mixed array_pad(mixed *arr, int size, mixed value, int beginning) ;

// File: base64
string base64_encode(mixed source_str) ;
string base64_decode(string source) ;

// File: data
varargs mixed data_value(string file, string key, mixed def) ;
varargs void data_write(string file, string key, mixed data...) ;
varargs int data_inc(string file, string key, int inc) ;
int data_del(string file, string key) ;

// File: description
string get_short(object ob) ;
string get_long(object ob) ;

// File: directory
int assure_dir(string path) ;
varargs string query_directory(object ob) ;

// File: english
string cap_words(string str) ;
varargs string cap_significant_words(string str, int title) ;
string possessive_noun(mixed ob) ;
string possessive_pronoun(mixed ob) ;
string possessive(mixed ob) ;
string reflexive(mixed ob) ;
string objective(mixed ob) ;
string subjective(mixed ob) ;
varargs string article(string str, int definite) ;
varargs string add_article(string str, int definite) ;
string remove_article(string str) ;

// File: exists
int directory_exists(string dirname) ;
int file_exists(string file) ;
int cfile_exists(string file) ;
int ofile_exists(string file) ;
int user_exists(string user) ;

// File: file
int file_lines(string file) ;
int log_file(string file, string str) ;
string *explode_file(string file) ;
varargs void implode_file(string file, string *lines, int overwrite) ;
string file_owner(string file) ;
string query_file_name(object ob) ;
varargs string tail(string path, int line_count) ;
void assure_file(string file) ;
varargs string temp_file(mixed arg) ;
string *dir_file(string path) ;
varargs string *valid_dir_file(string path, int file_too) ;
int touch(string file) ;

// File: function
int valid_function(mixed f) ;
mixed *assemble_call_back(mixed arg...) ;
mixed call_back(mixed *cb, mixed arg...) ;
varargs string call_trace(int colour) ;
mixed call_if(mixed ob, string func, mixed arg) ;
varargs int delay_act(string act, float delay, mixed *cb) ;
varargs void assert(mixed statement, string message) ;

// File: grammar
string int_string (int num) ;
string ordinal (int n) ;

// File: identify
string identify(mixed arg) ;

// File: json
mixed json_decode(string str) ;
mapping json_encode(mixed arg) ;

// File: mappings
string pretty_map(mapping map) ;

// File: messaging
varargs void tell_up(object ob, string str, int msg_type, mixed exclude) ;
varargs void tell_down(object ob, string str, int msg_type, mixed exclude) ;
varargs void tell_all(object ob, string str, int msg_type, mixed exclude) ;
varargs void tell_direct(object ob, string str, int msg_type, mixed exclude) ;
varargs void tell(object ob, string str, int msg_type, mixed exclude) ;

// File: numbers
float percent_of(float a, float b) ;
float percent(float a, float b) ;
float clamp(float min, float max, float val) ;
varargs float remainder(mixed a, mixed b) ;
int sum(mixed *arr) ;
int evaluate_number(int number, string condition) ;

// File: object
int caller_is(mixed ob) ;
int getoid(object ob) ;
int same_env_check(object ob1, object ob2, int top_env) ;
object *get_livings(mixed names, object room) ;
object *get_players(mixed names, object room) ;
object *present_clones(string file, object room) ;
object *present_livings(object room) ;
object *present_players(object room) ;
object *present_npcs(object room) ;
object get_living(string name, object room) ;
object get_player(string name, object room) ;
/**
 * @returns {BODY_OB}
 */
object this_body();
object top_environment(object ob) ;
varargs mixed get_objects(string str, object player, int no_arr) ;
varargs object find_ob(mixed ob, mixed cont) ;
varargs object get_object(string str, object player) ;
varargs object find_targets(object tp, string str, object env, function f) ;
varargs object find_target(object tp, string str, object env, function f) ;
varargs object *clones(mixed file, int env_only) ;

// File: prompt
varargs void prompt_colour(object body, mixed *cb, string prompt) ;
void prompt_password(object user, int attempts, mixed *cb) ;

// File: random
int *prandom(int seed, int max) ;
mixed *prandom_float(int seed) ;
int *prandom_clamp(int seed, int min, int max) ;
mixed *pelement_of(int seed, mixed *arr) ;
mixed *pelement_of_weighted(int seed, mapping weights) ;
float random_float(mixed upper_bound) ;
mixed element_of_weighted(mapping m) ;
int random_clamp(int min, int max) ;
int *sanitize_seed(mixed seed) ;

// File: resolve_path
string resolve_path(string base_dir, string path) ;
string valid_path(string base_dir, string path) ;
string valid_file(string base_dir, string path) ;
string valid_dir(string base_dir, string path) ;
string resolve_file(string base_dir, string path) ;
string resolve_dir(string base_dir, string path) ;

// File: save
string object_data_directory(object ob) ;
string object_data_file(object ob) ;
string assure_object_data_dir(object ob) ;

// File: security
int is_member(string user, string group) ;
int adminp(mixed user) ;
int devp(mixed user) ;
int wizardp(mixed user) ;

// File: signal
void emit(int sig, mixed arg...) ;
int slot(int sig, string func) ;
int unslot(int sig) ;

// File: socket
string dump_socket_status() ;

// File: string
string append(string source, string to_append) ;
string no_ansi(string str) ;
string prepend(string source, string to_prepend) ;
varargs string chop(string str, string sub, int dir) ;
varargs string extract(string str, int from, int to) ;
varargs string simple_list(mixed *arr, string conj) ;
varargs string substr(string str, string sub, int reverse) ;
varargs mixed from_string(string str, int flag) ;
string stringify(mixed val) ;
string add_commas(mixed number) ;
string reverse_string(string str) ;
string reverse_strsrch(string str, string sub, int start) ;
varargs int pcre_strsrch(string str, string pattern, int start) ;
int colourp(string str) ;

// File: system
int port() ;
object simul_efun() ;
mixed mud_config(string str) ;
string admin_email() ;
string arch() ;
string baselib_name() ;
string baselib_version() ;
string driver_version() ;
string lib_name() ;
string lib_version() ;
string mud_name() ;
string open_status() ;
varargs void _debug(string str, mixed args...) ;
int boot_number() ;
string log_dir() ;
string tmp_dir() ;
varargs int _ok(mixed args...) ;
varargs int _info(mixed args...) ;
varargs int _error(mixed args...) ;
varargs int _question(mixed args...) ;
varargs int _warn(mixed args...) ;

// File: time
varargs string ldate(int x, int brief) ;
varargs string ltime(int x, int brief) ;
int time_ms() ;
float time_frac(int nanoseconds) ;

// File: user
string user_data_file(string name) ;
string user_mob_data(string name) ;
string user_data_directory(string priv) ;
string home_path(string name) ;
string account_path(string name) ;
string account_file(string name) ;
int valid_account(string name) ;
int ghostp(object user) ;
int linkp(object user) ;
varargs int valid_user(mixed user) ;

// File: util
string generate_uuid() ;
int of(mixed needle, mixed haystack) ;

#endif // __SIMUL_EFUN_H__
