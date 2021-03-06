#include "o_connection_remote.h"
#include "o_native_socket_internal.h"
#include "o_storage_remote.h"
#include "o_input_stream_socket.h"
#include "o_output_stream_socket.h"
#include "o_native_lock.h"
#include "o_remote_protocol_specs.h"
#include "o_memory.h"
#include <string.h>
#include <netinet/in.h>
#include "o_exceptions.h"

struct o_connection_remote
{
	struct o_native_socket socket;
	struct o_input_stream * input;
	struct o_output_stream * output;
};

struct o_connection_remote * o_connection_remote_new_accept(struct o_native_socket * listen_sock)
{
	struct o_connection_remote * conn = o_malloc(sizeof(struct o_connection_remote));
	memset(conn, 0, sizeof(struct o_connection_remote));
	o_native_socket_accept_internal(listen_sock, &conn->socket);
	conn->input = o_input_stream_socket_new(&conn->socket);
	conn->output = o_output_stream_socket_new(&conn->socket);
	return conn;
}

struct o_connection_remote * o_connection_remote_new(char * host, int port)
{
	struct o_connection_remote * conn = o_malloc(sizeof(struct o_connection_remote));
	memset(conn, 0, sizeof(struct o_connection_remote));
	o_native_socket_connect_internal(&conn->socket, host, port);
	conn->input = o_input_stream_socket_new(&conn->socket);
	conn->output = o_output_stream_socket_new(&conn->socket);

	if (o_connection_remote_read_short(conn) != CURRENT_PROTOCOL)
		throw(o_exception_new("Wrong protocol version", 1));
	return conn;
}

void o_connection_remote_add_to_selector(struct o_connection_remote * connection, struct o_native_socket_selector * selector)
{
	o_native_socket_selector_add_socket(selector, &connection->socket);
}

void o_connection_remote_remove_from_selector(struct o_connection_remote * connection, struct o_native_socket_selector * selector)
{
	o_native_socket_selector_remove_socket(selector, &connection->socket);
}

int o_connection_remote_read_int(struct o_connection_remote * connection)
{
	int toRead = 0;
	o_input_stream_read_bytes(connection->input, &toRead, sizeof(toRead));
	return ntohl(toRead);
}

long long o_connection_remote_read_long64(struct o_connection_remote * connection)
{
	long long ret_val = 0;
	o_input_stream_read_bytes(connection->input, &ret_val, sizeof(ret_val));
	ret_val = (((long long) (ntohl((int) ((ret_val << 32) >> 32))) << 32) | (unsigned int) ntohl(((int) (ret_val >> 32))));
	return ret_val;
}

char o_connection_remote_read_byte(struct o_connection_remote * connection)
{
	char toRead = 0;
	o_input_stream_read_bytes(connection->input, &toRead, sizeof(toRead));
	return toRead;
}

short o_connection_remote_read_short(struct o_connection_remote * connection)
{
	short toRead = 0;
	o_input_stream_read_bytes(connection->input, &toRead, sizeof(toRead));
	return ntohs(toRead);
}

unsigned char * o_connection_remote_read_bytes(struct o_connection_remote * connection, int *byte_read)
{
	*byte_read = o_connection_remote_read_int(connection);
	if (*byte_read > 0)
	{
		unsigned char * bytes = o_malloc(((*byte_read) + 1) * sizeof(unsigned char));
		o_input_stream_read_bytes(connection->input, bytes, *byte_read);
		bytes[*byte_read] = 0;
		return bytes;
	}
	return 0;
}

char * o_connection_remote_read_string(struct o_connection_remote * connection)
{
	int byte_to_read = o_connection_remote_read_int(connection);
	char * bytes = o_malloc((byte_to_read + 1) * sizeof(char));
	o_input_stream_read_bytes(connection->input, bytes, byte_to_read);
	bytes[byte_to_read] = 0;
	return bytes;
}

char ** o_connection_remote_read_array_strings(struct o_connection_remote * connection, int *array_size)
{
	*array_size = o_connection_remote_read_int(connection);
	char **string_array = o_malloc(*array_size * sizeof(char *));
	int i;
	for (i = 0; i < *array_size; i++)
		string_array[i] = o_connection_remote_read_string(connection);
	return string_array;
}

void o_connection_remote_write_int(struct o_connection_remote * connection, int int_value)
{
	int_value = htonl(int_value);
	o_output_stream_write_bytes(connection->output, &int_value, sizeof(int_value));
}

void o_connection_remote_write_long64(struct o_connection_remote * connection, long long long_value)
{
	long_value = (((long long) (htonl((int) ((long_value << 32) >> 32))) << 32) | (unsigned int) htonl(((int) (long_value >> 32))));
	o_output_stream_write_bytes(connection->output, &long_value, sizeof(long_value));
}

void o_connection_remote_write_byte(struct o_connection_remote * connection, char byte_value)
{
	o_output_stream_write_bytes(connection->output, &byte_value, sizeof(byte_value));
}

void o_connection_remote_write_short(struct o_connection_remote * connection, short short_value)
{
	short_value = htons(short_value);
	o_output_stream_write_bytes(connection->output, &short_value, sizeof(short_value));
}

void o_connection_remote_write_bytes(struct o_connection_remote * connection, unsigned char *byte_array, int length)
{
	o_connection_remote_write_int(connection, length);
	o_output_stream_write_bytes(connection->output, byte_array, length);
}

void o_connection_remote_write_string(struct o_connection_remote * connection, const char *string)
{
	o_connection_remote_write_bytes(connection, (unsigned char *) string, strlen(string));
}

void o_connection_remote_flush(struct o_connection_remote * connection)
{
	o_output_stream_flush(connection->output);
}

void o_connection_remote_write_array_strings(struct o_connection_remote * connection, char **strings_array, int length)
{
	o_connection_remote_write_int(connection, length);
	int i;
	for (i = 0; i < length; i++)
		o_connection_remote_write_string(connection, strings_array[i]);
}

void o_connection_remote_free(struct o_connection_remote *connection)
{
	o_native_socket_close_internal(&connection->socket);
	o_input_stream_free(connection->input);
	o_output_stream_free(connection->output);
	o_free(connection);
}

