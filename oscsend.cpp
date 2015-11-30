
#define VERSION "0.26-svn"
#include <vector>
#include <sstream>

#include "oscsend.h"
using namespace std;
/*
  void usage(void)
  {
  printf("oscsend version %s\n"
  "Copyright (C) 2008 Kentaro Fukuchi\n\n"
  "Usage: oscsend hostname port address types values...\n"
  "Send OpenSound Control message via UDP.\n\n"
  "Description\n"
  "hostname: specifies the remote host's name.\n"
  "port    : specifies the port number to connect to the remote host.\n"
  "address : the OSC address where the message to be sent.\n"
  "types   : specifies the types of the following values.\n",
  VERSION);
  printf("          %c - 32bit integer\n", LO_INT32);
  printf("          %c - 64bit integer\n", LO_INT64);
  printf("          %c - 32bit floating point number\n", LO_FLOAT);
  printf("          %c - 64bit (double) floating point number\n",
  LO_DOUBLE);
  printf("          %c - string\n", LO_STRING);
  printf("          %c - symbol\n", LO_SYMBOL);
  printf("          %c - char\n", LO_CHAR);
  printf("          %c - 4 byte midi packet (8 digits hexadecimal)\n",
  LO_MIDI);
  printf("          %c - TRUE (no value required)\n", LO_TRUE);
  printf("          %c - FALSE (no value required)\n", LO_FALSE);
  printf("          %c - NIL (no value required)\n", LO_NIL);
  printf("          %c - INFINITUM (no value required)\n", LO_INFINITUM);
  printf("values  : space separated values.\n\n"
  "Example\n"
  "$ oscsend localhost 7777 /sample/address %c%c%c%c 1 3.14 hello\n",
  LO_INT32, LO_TRUE, LO_FLOAT, LO_STRING);
  }
*/
lo_message create_message(vector<string>& argv)
{
  /* Note:
   * argv[1] <- hostname
   * argv[2] <- port
   * argv[3] <- OSC address
   * argv[0] <- types
   * argv[1..] <- values
   */
  int i, argi;
  lo_message message;
  char types[1000], arg[1000];
  int values;
  message = lo_message_new();
  if (argv[0] == "") {
    /* empty message is allowed. */
    values = 0;
  } else {
    string word = argv[0];
    strcpy(types,word.c_str());
    values = strlen(types);
  }
    
  argi = 1;
  for (i = 0; i < values; i++) {
    if(argv[argi] != "")
      strcpy(arg,argv[argi].c_str());
    else {
      fprintf(stderr, "Value #%d is not given.\n", i + 1);
      lo_message_free(message);
      return NULL;
          
    }
    switch (types[i]) {
    case LO_INT32:
      {
	char *endp;
	int64_t v;

	v = strtol(arg, &endp, 10);
	if (*endp != '\0') {
	  fprintf(stderr, "An invalid value was given: '%s'\n",
		  arg);
	  lo_message_free(message);
	  return NULL;
                    
	}
	if ((v == LONG_MAX || v == LONG_MIN) && errno == ERANGE) {
	  fprintf(stderr, "Value out of range: '%s'\n", arg);
	  lo_message_free(message);
	  return NULL;
                    
	}
	if (v > INT_MAX || v < INT_MIN) {
	  fprintf(stderr, "Value out of range: '%s'\n", arg);
	  lo_message_free(message);
	  return NULL;
                    
	}
	lo_message_add_int32(message, (int32_t) v);
	argi++;
	break;
      }
    case LO_INT64:
      {
	char *endp;
	int64_t v;

	v = strtol(arg, &endp, 10);
	if (*endp != '\0') {
	  fprintf(stderr, "An invalid value was given: '%s'\n",
		  arg);
	  goto EXIT;
	}
	if ((v == LONG_MAX || v == LONG_MIN) && errno == ERANGE) {
	  fprintf(stderr, "Value out of range: '%s'\n", arg);
	  goto EXIT;
	}
	lo_message_add_int64(message, v);
	argi++;
	break;
      }
    case LO_FLOAT:
      {
	char *endp;
	float v;

#ifdef __USE_ISOC99
	v = strtof(arg, &endp);
#else
	v = (float) strtod(arg, &endp);
#endif                          /* __USE_ISOC99 */
	if (*endp != '\0') {
	  fprintf(stderr, "An invalid value was given: '%s'\n",
		  arg);
	  lo_message_free(message);
	  return NULL;
                    
	}
	lo_message_add_float(message, v);
	argi++;
	break;
      }
    case LO_DOUBLE:
      {
	char *endp;
	double v;

	v = strtod(arg, &endp);
	if (*endp != '\0') {
	  perror(NULL);
	  fprintf(stderr, "An invalid value was given: '%s'\n",
		  arg);
	  goto EXIT;
	}
	lo_message_add_double(message, v);
	argi++;
	break;
      }
    case LO_STRING:
      lo_message_add_string(message, arg);
      argi++;
      break;
    case LO_SYMBOL:
      lo_message_add_symbol(message, arg);
      argi++;
      break;
    case LO_CHAR:
      lo_message_add_char(message, arg[0]);
      argi++;
      break;
    case LO_MIDI:
      {
	unsigned int midi;
	uint8_t packet[4];
	int ret;

	ret = sscanf(arg, "%08x", &midi);
	if (ret != 1) {
	  fprintf(stderr,
		  "An invalid hexadecimal value was given: '%s'\n",
		  arg);
	  goto EXIT;
	}
	packet[0] = (midi >> 24) & 0xff;
	packet[1] = (midi >> 16) & 0xff;
	packet[2] = (midi >> 8) & 0xff;
	packet[3] = midi & 0xff;
	lo_message_add_midi(message, packet);
	argi++;
	break;
      }
    case LO_TRUE:
      lo_message_add_true(message);
      break;
    case LO_FALSE:
      lo_message_add_false(message);
      break;
    case LO_NIL:
      lo_message_add_nil(message);
      break;
    case LO_INFINITUM:
      lo_message_add_infinitum(message);
      break;
    default:
      fprintf(stderr, "Type '%c' is not supported or invalid.\n",
	      types[i]);
      lo_message_free(message);
      return NULL;
      break;
    }
  }

  return message;
 EXIT:	return NULL;
}

int OSC::init(string host, string port)
{
  //lo_address target;
  lo_message message;
  int ret;


  target = lo_address_new(host.c_str(),port.c_str());
  if (target == NULL) {
    fprintf(stderr, "Failed to open %s:%s\n", host.c_str(), port.c_str());
    return -1;
  }
  lo_address_set_ttl(target, 1);
  return 0;
}

void tokenizeMsg(string msg, vector<string>& words) {
   
  //    vector<string> words;
 
  stringstream ss(msg);
  string word;
  while (ss >> word)
    {
      words.push_back(word);
    }

}

int OSC::sendMsg(string msg, string serv) {

  vector<string> argv;
  tokenizeMsg(msg,argv);
  lo_message message = create_message(argv);
  if (message == NULL) {
    fprintf(stderr, "Failed to create OSC message.\n");
    return -1;
  }
  int ret;
  ret = lo_send_message(target, serv.c_str(), message);
  if (ret == -1) {
    fprintf(stderr, "An error occured: %s\n",
	    lo_address_errstr(target));
    return -1;
  }

  return 0;
}
