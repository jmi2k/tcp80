# tcp80

Just another HTTP server for Plan 9.

## Disclaimer

```c
int
validateuri(char path[], int method)
{
	USED(path);
	USED(method);
	return 1;
}
```

## Usage

```
% mk
% aux/listen1 tcp!*!8080 6.out	# /sys/www served at port 8080
```
