char server[]	= "tcp80/HEAD";
char index[]	= "index.html";
ulong timeout	= 60*1000;

Mimetype mimetypes[] = {
	{ ".css",	"text/css; charset=UTF-8" },
	{ ".txt",	"text/plain; charset=UTF-8" },
	{ ".html",	"text/html; charset=UTF-8" },
	{ ".jpeg",	"image/jpeg" },
	{ ".png",	"image/png" },
};
