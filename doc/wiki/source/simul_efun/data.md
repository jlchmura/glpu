# distinct_array

## SYNOPSIS

    mixed *distinct_array(mixed *arr)

### PARAMETERS

    mixed[] arr - An array of mixed types.

### RETURNS

    mixed[] - A new array with distinct elements from the input array.

## DESCRIPTION

Returns a new array containing the distinct elements of the input
array.


# remove_array_element

## SYNOPSIS

    varargs mixed *remove_array_element(mixed *arr, int start, int end)

### PARAMETERS

    mixed[] arr - The input array.
    int start - The starting index of elements to be removed.
    int [end] - The ending index of elements to be removed. Defaults to start if not specified.

### RETURNS

    mixed[] - A new array with specified elements removed.

## DESCRIPTION

Returns a new array containing the elements of the input array
from index 0 to start-1, and from end+1 to the end of the input
array. If start is greater than end, the new array will contain
all the elements of the input array.


# reverse_array

## SYNOPSIS

    mixed *reverse_array(mixed *arr)

### PARAMETERS

    mixed[] arr - The input array.

### RETURNS

    mixed[] - A new array with elements in reverse order.

## DESCRIPTION

Returns a new array with the elements of the input array in
reverse order.


# splice

## SYNOPSIS

    varargs mixed *splice(mixed *arr, int start, int delete_count, mixed *items_to_add)

### PARAMETERS

    mixed[] arr - The array from which elements will be removed and to which new elements may be added.
    int start - The zero-based index at which to start changing the array. If negative, it will begin that many elements from the end.
    int delete_count - The number of elements to remove from the array, starting from the index specified by start. If delete_count is 0, no elements are removed.
    mixed[] [items_to_add] - An array of elements to add to the array at the start index. Can be omitted or passed as null if no elements are to be added.

### RETURNS

    mixed[] - A new array reflecting the desired modifications.

## DESCRIPTION

Modifies the content of an array by removing existing elements
and/or adding new elements. Returns a new array with the
modifications.


# base64_decode

## SYNOPSIS

    string base64_decode(string source)

### PARAMETERS

    string source - The Base64 encoded string to be decoded.

### RETURNS

    string - The decoded string.

## DESCRIPTION

Decodes a given Base64 encoded string.


# base64_encode

## SYNOPSIS

    string base64_encode(mixed source_str)

### PARAMETERS

    mixed source_str - The string or buffer to be encoded.

### RETURNS

    string - The Base64 encoded string.

## DESCRIPTION

Encodes a given string or buffer into Base64 format.


# data_del

## SYNOPSIS

    int data_del(string file, string key)

### PARAMETERS

    string file - The file to modify.
    string key - The key to delete.

### RETURNS

    int - 1 if the key was found and deleted, 0 otherwise.

## DESCRIPTION

Deletes the key-value pair from the file.


# data_inc

## SYNOPSIS

    varargs int data_inc(string file, string key, int inc)

### PARAMETERS

    string file - The file to modify.
    string key - The key to increment the value for.
    int inc - The amount to increment by.

### RETURNS

    int - The new value after incrementing.

## DESCRIPTION

Increments the value associated with the given key in the file
by the specified amount. If the key does not exist, it is
created with the increment value.


# data_value

## SYNOPSIS

    varargs mixed data_value(string file, string key, mixed def)

### PARAMETERS

    string file - The file to read from.
    string key - The key to search for.
    mixed [def] - The default value to return if the key is not found.

### RETURNS

    mixed - The value associated with the key, or the default value if the key is not found.

## DESCRIPTION

Retrieves the value associated with a given key from a file.


# data_write

## SYNOPSIS

    varargs void data_write(string file, string key, mixed data...)

### PARAMETERS

    string file - The file to write to.
    string key - The key to write.
    mixed data - The value(s) to write.

## DESCRIPTION

Writes a key-value pair to a file. If the key already exists,
the value is updated.

