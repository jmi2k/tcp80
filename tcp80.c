#include <u.h>
#include <libc.h>
#include <bio.h>

#define PATHMAX	1024
#define LINEMAX	1024
#define RESMAX	8192

typedef struct Mimetype Mimetype;
typedef struct Req Req;
typedef struct Res Res;

struct Mimetype
{
	char*	ext;
	char*	mime;
};

struct Req
{
	int		method;
	char	uri[PATHMAX];
};

struct Res
{
	int		status;
	char*	mime;
	vlong	len;
};

enum
{
	Ok					= 200,
	PartialContent		= 206,
	MovedPermanently	= 301,
	NotModified			= 304,
	BadRequest			= 400,
	Forbidden			= 403,
	NotFound			= 404,
	MethodNotAllowed	= 405,
	RequestTimeout		= 408,
	UriTooLong			= 414,
	RequestTooLarge		= 431,
	InternalServerError = 500,
};

enum
{
	Get,
};

int lookupmethod(char[]);
char* lookupmime(char[]);
int validateuri(char[], int);
int recvheader(Req*);
void sendheader(Res*);
void serve(Req*, int);
void usage(void);

char *nstatus[] = {
	[Ok]					= "OK",
	[PartialContent]		= "Partial Content",
	[MovedPermanently]		= "Moved Permanently",
	[BadRequest]			= "Bad Request",
	[Forbidden]				= "Forbidden",
	[NotFound]				= "Not Found",
	[MethodNotAllowed]		= "Method Not Allowed",
	[RequestTimeout]		= "Request Timeout",
	[UriTooLong]			= "URI Too Long",
	[RequestTooLarge]		= "Request Too Large",
	[InternalServerError]	= "Internal Server Error",
};

char *nmethods[] = {
	[Get]	= "GET",
};

#include "config.h"
Biobufhdr in;

int
lookupmethod(char method[])
{
	int i;

	for(i = 0; i < nelem(nmethods); i++)
		if(cistrcmp(nmethods[i], method) == 0)
			return i;

	return -1;
}

char*
lookupmime(char ext[])
{
	Mimetype *t;
	int i;

	if(ext == nil)
		goto Default;

	for(i = 0; i < nelem(mimetypes); i++){
		t = mimetypes+i;

		if(strcmp(t->ext, ext) == 0)
			return t->mime;
	}

Default:
	return "application/octet-stream";
}

int
validateuri(char path[], int method)
{
	USED(path);
	USED(method);
	return 1;
}

int
recvheader(Req *req)
{
	char *line, *nmethod, *uri, *http;
	int method;

	alarm(timeout);

	if((line = Brdline(&in, '\n')) == nil)		/* no line */
		return BadRequest;
	if(line[Blinelen(&in)-1] != '\n')			/* line too long */
		return UriTooLong;

	if(!(nmethod = strtok(line, " ")))			/* no method */
		return BadRequest;
	if(!(uri = strtok(nil, " ")))				/* no uri */
		return BadRequest;
	if(!(http = strtok(nil, "\n\r")))			/* no HTTP version */
		return BadRequest;
	if(cistrcmp(http, "HTTP/1.1") != 0)			/* invalid HTTP version */
		return BadRequest;
	if(!(method = lookupmethod(nmethod)) < 0)	/* invalid method */
		return BadRequest;
	if(!validateuri(uri, method))				/* invalid uri */
		return BadRequest;

	alarm(0);
	req->method = method;
	strncpy(req->uri, uri, PATHMAX);
	return Ok;
}

void
sendheader(Res *res)
{
	print(
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %lld\r\n"
		"Server: %s\r\n"
		"\r\n",
		res->status, nstatus[res->status],
		res->mime,
		res->len,
		server);
}

void
serve(Req *req, int status)
{
	static uchar rbuf[RESMAX];
	Res res;
	int fd;
	vlong n;

	res.status = status;
	res.mime = lookupmime(strrchr(req->uri, '.'));

	if((fd = open(req->uri, OREAD)) < 0){
		res.status = NotFound;
		res.mime = "text/plain";
		res.len = strlen(nstatus[res.status]);
		sendheader(&res);
		print("%s", nstatus[res.status]);
	}else{
		res.len = seek(fd, 0, 2);
		seek(fd, 0, 0);
		sendheader(&res);
		while((n = read(fd, rbuf, RESMAX)) > 0)
			write(0, rbuf, n);
		close(fd);
	}
}

void
usage(void)
{
	fprint(2, "usage: %s [webroot]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	static uchar lbuf[LINEMAX];
	Req req;
	char *root;
	int status;

	root = "/sys/www";
	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc == 1)
		root = argv[0];
	else if(argc > 1)
		usage();

	if(Binits(&in, 0, OREAD, lbuf, LINEMAX) == Beof)
		sysfatal("Binit: %r");

	/* not bullet-proof but close enough */
	rfork(RFNAMEG|RFCENVG);
	if(bind(root, "/", MREPL) < 0)
		sysfatal("bind: %r");
	rfork(RFNOMNT);

	status = recvheader(&req);
	serve(&req, status);

	Bterm(&in);
}