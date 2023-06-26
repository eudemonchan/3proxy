/*
3APA3A simpliest proxy server
(c) 2002-2021 by Vladimir Dubrovin <3proxy@3proxy.org>

please read License Agreement

*/

#include "proxy.h"

#define RETURN(xxx)       \
	{                     \
		param->res = xxx; \
		goto CLEANRET;    \
	}

char *proxy_stringtable[] = {
	"HTTP/1.0 400 Bad Request\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>400 Bad Request</title></head>\r\n"
	"<body><h2>400 Bad Request</h2></body></html>\r\n",
	"HTTP/1.0 502 Bad Gateway\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>502 Bad Gateway</title></head>\r\n"
	"<body><h2>502 Bad Gateway</h2><h3>Host Not Found or connection failed</h3></body></html>\r\n",
	"HTTP/1.0 503 Service Unavailable\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>503 Service Unavailable</title></head>\r\n"
	"<body><h2>503 Service Unavailable</h2><h3>You have exceeded your traffic limit</h3></body></html>\r\n",
	"HTTP/1.0 503 Service Unavailable\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>503 Service Unavailable</title></head>\r\n"
	"<body><h2>503 Service Unavailable</h2><h3>Recursion detected</h3></body></html>\r\n",
	"HTTP/1.0 501 Not Implemented\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>501 Not Implemented</title></head>\r\n"
	"<body><h2>501 Not Implemented</h2><h3>Required action is not supported by proxy server</h3></body></html>\r\n",
	"HTTP/1.0 502 Bad Gateway\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>502 Bad Gateway</title></head>\r\n"
	"<body><h2>502 Bad Gateway</h2><h3>Failed to connect parent proxy</h3></body></html>\r\n",
	"HTTP/1.0 500 Internal Error\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>500 Internal Error</title></head>\r\n"
	"<body><h2>500 Internal Error</h2><h3>Internal proxy error during processing your request</h3></body></html>\r\n",
	"HTTP/1.0 407 Proxy Authentication Required\r\n"
	"Proxy-Authenticate: Basic realm=\"proxy\"\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>407 Proxy Authentication Required</title></head>\r\n"
	"<body><h2>407 Proxy Authentication Required</h2><h3>Access to requested resource disallowed by administrator or you need valid username/password to use this resource</h3></body></html>\r\n",
	"HTTP/1.0 200 Connection established\r\n\r\n",
	"HTTP/1.0 200 Connection established\r\n"
	"Content-Type: text/html\r\n\r\n",
	"HTTP/1.0 404 Not Found\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>404 Not Found</title></head>\r\n"
	"<body><h2>404 Not Found</h2><h3>File not found</body></html>\r\n",
	"HTTP/1.0 403 Forbidden\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>403 Access Denied</title></head>\r\n"
	"<body><h2>403 Access Denied</h2><h3>Access control list denies you to access this resource</body></html>\r\n",
	"HTTP/1.0 407 Proxy Authentication Required\r\n"
	"Proxy-Authenticate: NTLM\r\n"
	"Proxy-Authenticate: Basic realm=\"proxy\"\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>407 Proxy Authentication Required</title></head>\r\n"
	"<body><h2>407 Proxy Authentication Required</h2><h3>Access to requested resource disallowed by administrator or you need valid username/password to use this resource</h3></body></html>\r\n",
	"HTTP/1.0 407 Proxy Authentication Required\r\n"
	"Connection: keep-alive\r\n"
	"Content-Length: 0\r\n"
	"Proxy-Authenticate: NTLM ",
	"HTTP/1.0 403 Forbidden\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<pre>",
	"HTTP/1.0 503 Service Unavailable\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>503 Service Unavailable</title></head>\r\n"
	"<body><h2>503 Service Unavailable</h2><h3>Your request violates configured policy</h3></body></html>\r\n",
	"HTTP/1.0 401 Authentication Required\r\n"
	"WWW-Authenticate: Basic realm=\"FTP Server\"\r\n"
	"Connection: close\r\n"
	"Content-type: text/html; charset=utf-8\r\n"
	"\r\n"
	"<html><head><title>401 FTP Server requires authentication</title></head>\r\n"
	"<body><h2>401 FTP Server requires authentication</h2><h3>This FTP server rejects anonymous access</h3></body></html>\r\n",
	"HTTP/1.1 100 Continue\r\n"
	"\r\n",
	((void *)0)};
static void logurl(struct clientparam *param, char *buf, char *req, int ftp)
{
	char *sb;
	char *se;
	int len;
	if (!buf)
		req = ((void *)0);
	if (req)
	{
		len = (int)strlen(req);
		if (len > (32768 - 1))
			len = 32768 - 1;
		memcpy(buf, req, len + 1);
		buf[32768 - 1] = 0;
		sb = strchr(buf, '\r');
		if (sb)
			*sb = 0;
		if (ftp && (se = strchr(buf + 10, ':')) && (sb = strchr(se, '@')))
		{
			strcpy(se, sb);
		}
	}
	if (param->res != 555 && param->res != 508)
		dolog(param, (unsigned char *)(req ? buf : ((void *)0)));
}
void decodeurl(unsigned char *s, int allowcr)
{
	unsigned char *d = s;
	unsigned u;
	while (*s)
	{
		if (*s == '%' && ((s[1] >= '0' && s[1] <= '9') || (s[1] >= 'a' && s[1] <= 'f') || (s[1] >= 'A' && s[1] <= 'F')) && ((s[2] >= '0' && s[2] <= '9') || (s[2] >= 'a' && s[2] <= 'f') || (s[2] >= 'A' && s[2] <= 'F')))
		{
			sscanf((char *)s + 1, "%2x", &u);
			if (allowcr && u != '\r')
				*d++ = u;
			else if (u != '\r' && u != '\n')
			{
				if (u == '\"' || u == '\\')
					*d++ = '\\';
				else if (u == 255)
					*d++ = 255;
				*d++ = u;
			}
			s += 3;
		}
		else if (!allowcr && *s == '?')
		{
			break;
		}
		else if (*s == '+')
		{
			*d++ = ' ';
			s++;
		}
		else
		{
			*d++ = *s++;
		}
	}
	*d = 0;
}
void file2url(unsigned char *sb, unsigned char *buf, unsigned bufsize, int *inbuf, int skip255)
{
	for (; *sb; sb++)
	{
		if ((bufsize - *inbuf) < 16)
			break;
		if (*sb == '\r' || *sb == '\n')
			continue;
		if (((*sb >= '0' && *sb <= '9') || (*sb >= 'a' && *sb <= 'z') || (*sb >= 'A' && *sb <= 'Z') || (*sb >= '*' && *sb <= '/') || *sb == '_'))
			buf[(*inbuf)++] = *sb;
		else if (*sb == '\"')
		{
			memcpy(buf + *inbuf, "%5C%22", 6);
			(*inbuf) += 6;
		}
		else if (skip255 && *sb == 255 && *(sb + 1) == 255)
		{
			memcpy(buf + *inbuf, "%ff", 3);
			(*inbuf) += 3;
			sb++;
		}
		else
		{
			sprintf((char *)buf + *inbuf, "%%%.2x", (unsigned)*sb);
			(*inbuf) += 3;
		}
	}
}
void *proxychild(struct clientparam *param)
{
	int res = 0, i = 0;
	unsigned char *buf = ((void *)0), *newbuf;
	int inbuf;
	int bufsize;
	unsigned reqlen = 0;
	unsigned char *sb = ((void *)0), *sg = ((void *)0), *se = ((void *)0), *sp = ((void *)0),
				  *req = ((void *)0), *su = ((void *)0), *ss = ((void *)0);
	unsigned char *ftpbase = ((void *)0);
	unsigned char username[1024];
	int keepalive = 0;
	uint64_t contentlength64 = 0;
	int hascontent = 0;
	int isconnect = 0;
	int redirect = 0;
	int prefix = 0, ckeepalive = 0;
	int ftp = 0;
	int anonymous;
	int sleeptime = 0;
	int reqsize, reqbufsize;
	int authenticate;
	struct pollfd fds[2];
	SOCKET ftps;
	char ftpbuf[1536];
	int inftpbuf = 0;
	int haveconnection = 0;
	if (param->remsock != (SOCKET)(~0))
		haveconnection = 1;
	if (!(buf = malloc((32768 * 2))))
	{
		{
			param->res = 21;
			goto CLEANRET;
		};
	}
	bufsize = (32768 * 2);
	anonymous = param->srv->anonymous;
	for (;;)
	{
		memset(buf, 0, bufsize);
		inbuf = 0;
		if (keepalive && (param->cliinbuf == param->clioffset) && (param->remsock != (SOCKET)(~0)))
		{
			memset(fds, 0, sizeof(fds));
			fds[0].fd = param->clisock;
			fds[0].events = (0x0100 | 0x0200);
			fds[1].fd = param->remsock;
			fds[1].events = (0x0100 | 0x0200);
			res = so._poll(fds, 2, conf.timeouts[STRING_S] * 1000);
			if (res <= 0)
			{
				{
					param->res = 555;
					goto CLEANRET;
				};
			}
			if ((fds[1].revents & ((0x0100 | 0x0200) | 0x0002 | 0x0001 | 0x0004)))
			{
				if (param->transparent || (!param->redirected && param->redirtype == R_HTTP))
				{
					param->res = 555;
					goto CLEANRET;
				};
				ckeepalive = 0;
				so._shutdown(param->remsock, 0x02);
				so._closesocket(param->remsock);
				param->remsock = (SOCKET)(~0);
				param->redirected = 0;
				param->redirtype = 0;
				memset(&param->sinsl, 0, sizeof(param->sinsl));
				memset(&param->sinsr, 0, sizeof(param->sinsr));
				memset(&param->req, 0, sizeof(param->req));
			}
		}
		i = sockgetlinebuf(param, CLIENT, buf, 32768 - 1, '\n', conf.timeouts[STRING_L]);
		if (i <= 0)
		{
			{
				param->res = (keepalive) ? 555 : (i) ? 507
													 : 508;
				goto CLEANRET;
			};
		}
		if (i == 2 && buf[0] == '\r' && buf[1] == '\n')
			continue;
		buf[i] = 0;
		if (req)
		{
			if (!param->transparent && !param->srv->transparent && (i <= prefix || strnicmp((char *)buf, (char *)req, prefix)))
			{
				ckeepalive = 0;
				if (param->remsock != (SOCKET)(~0))
				{
					so._shutdown(param->remsock, 0x02);
					so._closesocket(param->remsock);
				}
				param->remsock = (SOCKET)(~0);
				param->redirected = 0;
				param->redirtype = 0;
				memset(&param->sinsl, 0, sizeof(param->sinsl));
				memset(&param->sinsr, 0, sizeof(param->sinsr));
				memset(&param->req, 0, sizeof(param->req));
			}
			free(req);
		}
		req = (unsigned char *)strdup((char *)buf);
		if (!req)
		{
			{
				param->res = 510;
				goto CLEANRET;
			};
		}
		if (i < 10)
		{
			{
				param->res = 511;
				goto CLEANRET;
			};
		}
		if (buf[i - 3] == '1')
			keepalive = 2;
		param->transparent = 0;
		if ((isconnect = !strnicmp((char *)buf, "CONNECT", 7)))
			keepalive = 2;
		if ((sb = (unsigned char *)(unsigned char *)strchr((char *)buf, ' ')) == ((void *)0))
		{
			{
				param->res = 512;
				goto CLEANRET;
			};
		}
		ss = ++sb;
		if (!isconnect)
		{
			if (!strnicmp((char *)sb, "http://", 7))
			{
				sb += 7;
			}
			else if (!strnicmp((char *)sb, "ftp://", 6))
			{
				ftp = 1;
				sb += 6;
			}
			else if (*sb == '/')
			{
				param->transparent = 1;
			}
			else
			{
				{
					param->res = 513;
					goto CLEANRET;
				};
			}
		}
		else
		{
			if ((se = (unsigned char *)(unsigned char *)strchr((char *)sb, ' ')) == ((void *)0) || sb == se)
			{
				{
					param->res = 514;
					goto CLEANRET;
				};
			}
			*se = 0;
		}
		if (!param->transparent || isconnect)
		{
			if (!isconnect)
			{
				if ((se = (unsigned char *)(unsigned char *)strchr((char *)sb, '/')) == ((void *)0) || sb == se || !(sg = (unsigned char *)strchr((char *)sb, ' ')))
				{
					{
						param->res = 515;
						goto CLEANRET;
					};
				}
				if (se > sg)
					se = sg;
				*se = 0;
			}
			prefix = (int)(se - buf);
			su = (unsigned char *)strrchr((char *)sb, '@');
			if (su)
			{
				su = (unsigned char *)strdup((char *)sb);
				decodeurl(su, 0);
				if (parseconnusername((char *)su, (struct clientparam *)param, 1, (unsigned short)((ftp) ? 21 : 80)))
				{
					param->res = 100;
					goto CLEANRET;
				};
				free(su);
			}
			else if (parsehostname((char *)sb, (struct clientparam *)param, (unsigned short)((ftp) ? 21 : 80)))
			{
				param->res = 100;
				goto CLEANRET;
			};
			if (!isconnect)
			{
				if (se == sg)
					*se-- = ' ';
				*se = '/';
				memmove(ss, se, i - (se - sb) + 1);
			}
		}
		reqlen = i = (int)strlen((char *)buf);
		if (!strnicmp((char *)buf, "CONNECT", 7))
			param->operation = 0x00001000;
		else if (!strnicmp((char *)buf, "GET", 3))
			param->operation = (ftp) ? 0x00010000 : 0x00000100;
		else if (!strnicmp((char *)buf, "PUT", 3))
			param->operation = (ftp) ? 0x00020000 : 0x00000200;
		else if (!strnicmp((char *)buf, "POST", 4) || !strnicmp((char *)buf, "BITS_POST", 9))
			param->operation = 0x00000400;
		else if (!strnicmp((char *)buf, "HEAD", 4))
			param->operation = 0x00000800;
		else
			param->operation = 0x00008000;
		do
		{
			buf[inbuf + i] = 0;
			if (!isconnect && ((i > 25 && !strnicmp((char *)(buf + inbuf), "proxy-connection:", 17)) ||
							   (i > 16 && (!strnicmp((char *)(buf + inbuf), "connection:", 11)))))
			{
				sb = (unsigned char *)strchr((char *)(buf + inbuf), ':');
				if (!sb)
					continue;
				++sb;
				while (isspace(*sb))
					sb++;
				if (strnicmp((char *)sb, "upgrade", 7))
				{
					if (!strnicmp((char *)sb, "keep-alive", 10))
						keepalive = 1;
					else
						keepalive = 0;
					continue;
				}
			}
			if (i > 11 && !strnicmp((char *)(buf + inbuf), "Expect: 100", 11))
			{
				keepalive = 1;
				socksend(param->clisock, (unsigned char *)proxy_stringtable[17], (int)strlen(proxy_stringtable[17]), conf.timeouts[STRING_S]);
				continue;
			}
			if (param->transparent && i > 6 && !strnicmp((char *)buf + inbuf, "Host:", 5))
			{
				unsigned char c;
				sb = (unsigned char *)strchr((char *)(buf + inbuf), ':');
				if (!sb)
					continue;
				++sb;
				while (isspace(*sb))
					sb++;
				(se = (unsigned char *)strchr((char *)sb, '\r')) || (se = (unsigned char *)strchr((char *)sb, '\n'));
				if (se)
				{
					c = *se;
					*se = 0;
				}
				if (!param->hostname)
				{
					if (parsehostname((char *)sb, param, 80))
					{
						param->res = 100;
						goto CLEANRET;
					};
				}
				newbuf = malloc(strlen((char *)req) + strlen((char *)(buf + inbuf)) + 8);
				if (newbuf)
				{
					sp = (unsigned char *)strchr((char *)req + 1, '/');
					memcpy(newbuf, req, (sp - req));
					sprintf((char *)newbuf + (sp - req), "http://%s%s", sb, sp);
					free(req);
					req = newbuf;
				}
				if (se)
					*se = c;
			}
			if (ftp && i > 13 && (!strnicmp((char *)(buf + inbuf), "authorization", 13)))
			{
				sb = (unsigned char *)strchr((char *)(buf + inbuf), ':');
				if (!sb)
					continue;
				++sb;
				while (isspace(*sb))
					sb++;
				if (!*sb)
					continue;
				if (!strnicmp((char *)sb, "basic", 5))
				{
					sb += 5;
					while (isspace(*sb))
						sb++;
					i = de64(sb, username, 255);
					if (i <= 0)
						continue;
					username[i] = 0;
					sb = (unsigned char *)strchr((char *)username, ':');
					if (sb)
					{
						*sb = 0;
						if (param->extpassword)
							free(param->extpassword);
						param->extpassword = (unsigned char *)strdup((char *)sb + 1);
					}
					if (param->extusername)
						free(param->extusername);
					param->extusername = (unsigned char *)strdup((char *)username);
					continue;
				}
			}
			if (i > 15 && (!strnicmp((char *)(buf + inbuf), "content-length", 14)))
			{
				sb = (unsigned char *)strchr((char *)(buf + inbuf), ':');
				if (!sb)
					continue;
				++sb;
				while (isspace(*sb))
					sb++;
				sscanf((char *)sb, "%"
								   "I64"
								   "u",
					   &contentlength64);
				if (param->maxtrafout64 && (param->maxtrafout64 < param->statscli64 || contentlength64 > param->maxtrafout64 - param->statscli64))
				{
					{
						param->res = 10;
						goto CLEANRET;
					};
				}
				if (param->ndatfilterscli > 0 && contentlength64 > 0)
					continue;
			}
			inbuf += i;
			if ((bufsize - inbuf) < 32768)
			{
				if (bufsize > (32768 * 16))
				{
					{
						param->res = 516;
						goto CLEANRET;
					};
				}
				if (!(newbuf = realloc(buf, bufsize + (32768 * 2))))
				{
					{
						param->res = 21;
						goto CLEANRET;
					};
				}
				buf = newbuf;
				bufsize += (32768 * 2);
			}
		} while ((i = sockgetlinebuf(param, CLIENT, buf + inbuf, 32768 - 2, '\n', conf.timeouts[STRING_S])) > 2);
		buf[inbuf] = 0;
		reqsize = (int)strlen((char *)req);
		reqbufsize = reqsize + 1;
		if (param->srv->needuser > 1 && !param->username)
		{
			{
				param->res = 4;
				goto CLEANRET;
			};
		}
		if ((res = (*param->srv->authfunc)(param)))
		{
			if (res <= 10 || haveconnection || param->transparent)
			{
				param->res = res;
				goto CLEANRET;
			};
			so._closesocket(param->remsock);
			param->remsock = (SOCKET)(~0);
			param->redirected = 0;
			param->redirtype = 0;
			memset(&param->sinsl, 0, sizeof(param->sinsl));
			memset(&param->sinsr, 0, sizeof(param->sinsr));
			if ((res = (*param->srv->authfunc)(param)))
			{
				param->res = res;
				goto CLEANRET;
			};
		}
		if (ftp && param->redirtype != R_HTTP)
		{
			SOCKET s;
			int mode = 0;
			int i = 0;
			inftpbuf = 0;
			if (!ckeepalive)
			{
				inftpbuf = 1536 - 20;
				res = ftplogin(param, ftpbuf, &inftpbuf);
				if (res)
				{
					{
						param->res = res;
						goto CLEANRET;
					};
				}
			}
			ckeepalive = 1;
			if (ftpbase)
				free(ftpbase);
			ftpbase = ((void *)0);
			if (!(sp = (unsigned char *)strchr((char *)ss, ' ')))
			{
				{
					param->res = 799;
					goto CLEANRET;
				};
			}
			*sp = 0;
			decodeurl(ss, 0);
			i = (int)strlen((char *)ss);
			if (!(ftpbase = malloc(i + 2)))
			{
				{
					param->res = 21;
					goto CLEANRET;
				};
			}
			memcpy(ftpbase, ss, i);
			if (ftpbase[i - 1] != '/')
				ftpbase[i++] = '/';
			ftpbase[i] = 0;
			memcpy(buf, "<pre><hr>\n", 10);
			inbuf = 10;
			if (inftpbuf)
			{
				memcpy(buf + inbuf, ftpbuf, inftpbuf);
				inbuf += inftpbuf;
				memcpy(buf + inbuf, "<hr>", 4);
				inbuf += 4;
			}
			if (ftpbase[1] != 0)
			{
				memcpy(buf + inbuf, "[<A HREF=\"..\">..</A>]\n", 22);
				inbuf += 22;
			}
			inftpbuf = 1536 - (20 + inftpbuf);
			res = ftpcd(param, ftpbase, ftpbuf, &inftpbuf);
			if (res)
			{
				res = ftptype(param, (unsigned char *)"I");
				if (res)
				{
					param->res = res;
					goto CLEANRET;
				};
				ftpbase[--i] = 0;
				ftps = ftpcommand(param, param->operation == 0x00020000 ? (unsigned char *)"PUT" : (unsigned char *)"RETR", ftpbase);
			}
			else
			{
				if (inftpbuf)
				{
					memcpy(buf + inbuf, ftpbuf, inftpbuf);
					inbuf += inftpbuf;
					memcpy(buf + inbuf, "<hr>", 4);
					inbuf += 4;
				}
				ftps = ftpcommand(param, (unsigned char *)"LIST", ((void *)0));
				mode = 1;
			}
			if (ftps == (SOCKET)(~0))
			{
				{
					param->res = 780;
					goto CLEANRET;
				};
			}
			if (!mode)
			{
				socksend(param->clisock, (unsigned char *)proxy_stringtable[8], (int)strlen(proxy_stringtable[8]), conf.timeouts[STRING_S]);
				s = param->remsock;
				param->remsock = ftps;
				if ((param->operation == 0x00020000) && (contentlength64 > 0))
					param->waitclient64 = contentlength64;
				res = sockmap(param, conf.timeouts[CONNECTION_L], 0);
				if (res == 99)
					res = 0;
				so._closesocket(ftps);
				ftps = (SOCKET)(~0);
				param->remsock = s;
			}
			else
			{
				int headsent = 0;
				int gotres = -1;
				s = param->remsock;
				if (param->srvoffset < param->srvinbuf)
				{
					gotres = ftpres(param, buf + inbuf, bufsize - (inbuf + 100));
					if (gotres)
						inbuf = (int)strlen((char *)buf);
				}
				param->remsock = ftps;
				if (gotres <= 0)
					for (; (res = sockgetlinebuf(param, SERVER, (unsigned char *)ftpbuf, 1536 - 20, '\n', conf.timeouts[STRING_S])) > 0; i++)
					{
						int isdir = 0;
						int islink = 0;
						int filetoken = -1;
						int sizetoken = -1;
						int modetoken = -1;
						int datetoken = -1;
						int spaces = 1;
						unsigned char *tokens[10];
						unsigned wordlen[10];
						unsigned char j = 0;
						int space = 1;
						ftpbuf[res] = 0;
						if (!i && ftpbuf[0] == 't' && ftpbuf[1] == 'o' && ftpbuf[2] == 't')
						{
							mode = 2;
							continue;
						}
						if (!(*ftpbuf >= '0' && *ftpbuf <= '9') && mode == 1)
							mode = 2;
						for (sb = (unsigned char *)ftpbuf; *sb; sb++)
						{
							if (!space && isspace(*sb))
							{
								space = 1;
								wordlen[j] = (unsigned)(sb - tokens[j]);
								j++;
							}
							if (space && !isspace(*sb))
							{
								space = 0;
								tokens[j] = sb;
								if (j == 8)
									break;
							}
						}
						if (mode == 1)
						{
							if (j < 4)
								continue;
							if (!(isdir = !memcmp(tokens[2], "<DIR>", wordlen[2])) && !(*tokens[2] >= '0' && *tokens[2] <= '9'))
							{
								continue;
							}
							datetoken = 0;
							wordlen[datetoken] = ((unsigned)(tokens[1] - tokens[0])) + wordlen[1];
							sizetoken = 2;
							filetoken = 3;
							spaces = 10;
						}
						else
						{
							if (j < 8 || wordlen[0] != 10)
								continue;
							if (j < 8 || !(*tokens[4] >= '0' && *tokens[4] <= '9'))
								mode = 3;
							if (*tokens[0] == 'd')
								isdir = 1;
							if (*tokens[0] == 'l')
								islink = 1;
							modetoken = 0;
							sizetoken = (mode == 2) ? 4 : 3;
							filetoken = (mode == 2) ? 8 : 7;
							datetoken = (mode == 2) ? 5 : 4;
							tokens[filetoken] = tokens[filetoken - 1];
							while (*tokens[filetoken] && !isspace(*tokens[filetoken]))
								tokens[filetoken]++;
							if (*tokens[filetoken])
							{
								tokens[filetoken]++;
							}
							wordlen[datetoken] = (unsigned)(tokens[filetoken] - tokens[datetoken]);
							wordlen[filetoken] = (unsigned)strlen((char *)tokens[filetoken]);
						}
						if (modetoken >= 0)
							memcpy(buf + inbuf, tokens[modetoken], 11);
						else
							memcpy(buf + inbuf, "---------- ", 11);
						inbuf += 11;
						if ((int)wordlen[datetoken] + 256 > bufsize - inbuf)
							continue;
						memcpy(buf + inbuf, tokens[datetoken], wordlen[datetoken]);
						inbuf += wordlen[datetoken];
						if (isdir)
						{
							memcpy(buf + inbuf, "       DIR", 10);
							inbuf += 10;
						}
						else if (islink)
						{
							memcpy(buf + inbuf, "      LINK", 10);
							inbuf += 10;
						}
						else
						{
							unsigned k;
							if (wordlen[sizetoken] > 10)
								wordlen[sizetoken] = 10;
							for (k = 10; k > wordlen[sizetoken]; k--)
							{
								buf[inbuf++] = ' ';
							}
							memcpy(buf + inbuf, tokens[sizetoken], wordlen[sizetoken]);
							inbuf += wordlen[sizetoken];
						}
						memcpy(buf + inbuf, " <A HREF=\"", 10);
						inbuf += 10;
						sb = ((void *)0);
						if (islink)
							sb = (unsigned char *)strstr((char *)tokens[filetoken], " -> ");
						if (sb)
							sb += 4;
						else
							sb = tokens[filetoken];
						if (*sb != '/' && ftpbase)
							file2url(ftpbase, buf, bufsize, (int *)&inbuf, 1);
						file2url(sb, buf, bufsize, (int *)&inbuf, 0);
						if (isdir)
							buf[inbuf++] = '/';
						memcpy(buf + inbuf, "\">", 2);
						inbuf += 2;
						for (sb = tokens[filetoken]; *sb; sb++)
						{
							if ((bufsize - inbuf) < 16)
								break;
							if (*sb == '<')
							{
								memcpy(buf + inbuf, "&lt;", 4);
								inbuf += 4;
							}
							else if (*sb == '>')
							{
								memcpy(buf + inbuf, "&gt;", 4);
								inbuf += 4;
							}
							else if (*sb == '\r' || *sb == '\n')
							{
								continue;
							}
							else if (islink && sb[0] == ' ' && sb[1] == '-' && sb[2] == '>')
							{
								memcpy(buf + inbuf, "</A> ", 5);
								inbuf += 5;
							}
							else
								buf[inbuf++] = *sb;
						}
						if (islink != 2)
						{
							memcpy(buf + inbuf, "</A>", 4);
							inbuf += 4;
						}
						buf[inbuf++] = '\n';
						if ((bufsize - inbuf) < 32768)
						{
							if (bufsize > 20000)
							{
								if (!headsent++)
								{
									socksend(param->clisock, (unsigned char *)proxy_stringtable[9], (int)strlen(proxy_stringtable[9]), conf.timeouts[STRING_S]);
								}
								if ((unsigned)socksend(param->clisock, buf, inbuf, conf.timeouts[STRING_S]) != inbuf)
								{
									{
										param->res = 781;
										goto CLEANRET;
									};
								}
								inbuf = 0;
							}
							else
							{
								if (!(newbuf = realloc(buf, bufsize + (32768 * 2))))
								{
									{
										param->res = 21;
										goto CLEANRET;
									};
								}
								buf = newbuf;
								bufsize += (32768 * 2);
							}
						}
					}
				memcpy(buf + inbuf, "<hr>", 4);
				inbuf += 4;
				so._closesocket(ftps);
				ftps = (SOCKET)(~0);
				param->remsock = s;
				if (inbuf)
				{
					buf[inbuf] = 0;
					if (gotres < 0)
						res = ftpres(param, buf + inbuf, bufsize - inbuf);
					else
						res = gotres;
					inbuf = (int)strlen((char *)buf);
					if (!headsent)
					{
						sprintf(ftpbuf,
								"HTTP/1.0 200 OK\r\n"
								"Content-Type: text/html\r\n"
								"Connection: keep-alive\r\n"
								"Content-Length: %d\r\n\r\n",
								inbuf);
						socksend(param->clisock, (unsigned char *)ftpbuf, (int)strlen(ftpbuf), conf.timeouts[STRING_S]);
					}
					socksend(param->clisock, buf, inbuf, conf.timeouts[STRING_S]);
					if (res)
					{
						{
							param->res = res;
							goto CLEANRET;
						};
					}
					if (!headsent)
						goto REQUESTEND;
				}
				{
					param->res = 0;
					goto CLEANRET;
				};
			}
			{
				param->res = res;
				goto CLEANRET;
			};
		}
		if (isconnect && param->redirtype != R_HTTP)
		{
			if (param->redirectfunc)
			{
				if (req)
					free(req);
				if (buf)
					free(buf);
				return (*param->redirectfunc)(param);
			}
			param->res = sockmap(param, conf.timeouts[CONNECTION_L], 0);
			if (param->redirectfunc)
			{
				if (req)
					free(req);
				if (buf)
					free(buf);
				return (*param->redirectfunc)(param);
			}
			{
				param->res = param->res;
				goto CLEANRET;
			};
		}
		if (!req || param->redirtype != R_HTTP)
		{
			reqlen = 0;
		}
		else
		{
			redirect = 1;
			res = (int)strlen((char *)req);
			if (socksend(param->remsock, req, res, conf.timeouts[STRING_L]) != res)
			{
				{
					param->res = 518;
					goto CLEANRET;
				};
			}
			param->statscli64 += res;
			param->nwrites++;
		}
		inbuf = 0;
		if (keepalive <= 1)
		{
			sprintf((char *)buf + strlen((char *)buf), "Connection: %s\r\n", keepalive ? "keep-alive" : "close");
		}
		if (param->extusername)
		{
			sprintf((char *)buf + strlen((char *)buf), "%s: Basic ", (redirect) ? "Proxy-Authorization" : "Authorization");
			sprintf((char *)username, "%.128s:%.128s", param->extusername, param->extpassword ? param->extpassword : (unsigned char *)"");
			en64(username, buf + strlen((char *)buf), (int)strlen((char *)username));
			sprintf((char *)buf + strlen((char *)buf), "\r\n");
		}
		sprintf((char *)buf + strlen((char *)buf), "\r\n");
		if ((res = socksend(param->remsock, buf + reqlen, (int)strlen((char *)buf + reqlen), conf.timeouts[STRING_S])) != (int)strlen((char *)buf + reqlen))
		{
			{
				param->res = 518;
				goto CLEANRET;
			};
		}
		param->statscli64 += res;
		param->nwrites++;
		if (param->bandlimfunc)
		{
			sleeptime = param->bandlimfunc(param, 0, (int)strlen((char *)buf));
		}
		if (contentlength64 > 0)
		{
			param->waitclient64 = contentlength64;
			res = sockmap(param, conf.timeouts[CONNECTION_S], 0);
			param->waitclient64 = 0;
			if (res != 99)
			{
				{
					param->res = res;
					goto CLEANRET;
				};
			}
		}
		contentlength64 = 0;
		inbuf = 0;
		ckeepalive = keepalive;
		res = 0;
		authenticate = 0;
		param->chunked = 0;
		hascontent = 0;
		while ((i = sockgetlinebuf(param, SERVER, buf + inbuf, 32768 - 1, '\n', conf.timeouts[(res) ? STRING_S : STRING_L])) > 2)
		{
			if (!res && i > 9)
				param->status = res = atoi((char *)buf + inbuf + 9);
			if (((i >= 25 && !strnicmp((char *)(buf + inbuf), "proxy-connection:", 17)) ||
				 (i > 16 && !strnicmp((char *)(buf + inbuf), "connection:", 11))))
			{
				sb = (unsigned char *)strchr((char *)(buf + inbuf), ':');
				if (!sb)
					continue;
				++sb;
				while (isspace(*sb))
					sb++;
				if (strnicmp((char *)sb, "keep-alive", 10))
					ckeepalive = 0;
				if (!param->srv->transparent && res >= 200)
					continue;
			}
			else if (i > 6 && !param->srv->transparent && (!strnicmp((char *)(buf + inbuf), "proxy-", 6)))
			{
				continue;
			}
			else if (i > 6 && (!strnicmp((char *)(buf + inbuf), "www-authenticate", 16)))
			{
				authenticate = 1;
			}
			else if (i > 15 && (!strnicmp((char *)(buf + inbuf), "content-length", 14)))
			{
				buf[inbuf + i] = 0;
				sb = (unsigned char *)strchr((char *)(buf + inbuf), ':');
				if (!sb)
					continue;
				++sb;
				while (isspace(*sb))
					sb++;
				sscanf((char *)sb, "%"
								   "I64"
								   "u",
					   &contentlength64);
				hascontent = 1;
				if (param->unsafefilter && param->ndatfilterssrv > 0)
				{
					hascontent = 2;
					continue;
				}
				if (param->maxtrafin64 && (param->maxtrafin64 < param->statssrv64 || contentlength64 + param->statssrv64 > param->maxtrafin64))
				{
					{
						param->res = 10;
						goto CLEANRET;
					};
				}
			}
			else if (i > 25 && (!strnicmp((char *)(buf + inbuf), "transfer-encoding", 17)))
			{
				buf[inbuf + i] = 0;
				sb = (unsigned char *)strchr((char *)(buf + inbuf), ':');
				if (!sb)
					continue;
				++sb;
				while (isspace(*sb))
					sb++;
				if (!strnicmp((char *)sb, "chunked", 7))
				{
					param->chunked = 1;
				}
			}
			inbuf += i;
			if ((bufsize - inbuf) < 32768)
			{
				if (bufsize > 20000)
				{
					{
						param->res = 516;
						goto CLEANRET;
					};
				}
				if (!(newbuf = realloc(buf, bufsize + (32768 * 2))))
				{
					{
						param->res = 21;
						goto CLEANRET;
					};
				}
				buf = newbuf;
				bufsize += (32768 * 2);
			}
		}
		if (res < 200 || res > 499)
		{
			ckeepalive = 0;
		}
		if ((res == 304 || res == 204) && !hascontent)
		{
			hascontent = 1;
			contentlength64 = 0;
		}
		if (param->bandlimfunc)
		{
			int st1;
			st1 = (*param->bandlimfunc)(param, inbuf, 0);
			if (st1 > sleeptime)
				sleeptime = st1;
			if (sleeptime > 0)
			{
				Sleep(sleeptime * 1);
			}
		}
		buf[inbuf] = 0;
		if (inbuf < 9)
		{
			{
				param->res = 522;
				goto CLEANRET;
			};
		}
		if (!isconnect || param->operation)
		{
			if (authenticate && !param->transparent)
				sprintf((char *)buf + strlen((char *)buf),
						"Proxy-support: Session-Based-Authentication\r\n"
						"Connection: Proxy-support\r\n");
			if (!param->srv->transparent && res >= 200)
			{
				if (ckeepalive <= 1)
					sprintf((char *)buf + strlen((char *)buf), "Connection: %s\r\n",
							(hascontent && ckeepalive) ? "keep-alive" : "close");
			}
			sprintf((char *)buf + strlen((char *)buf), "\r\n");
			if ((socksend(param->clisock, buf, (int)strlen((char *)buf), conf.timeouts[STRING_S])) != (int)strlen((char *)buf))
			{
				{
					param->res = 521;
					goto CLEANRET;
				};
			}
		}
		if ((param->chunked || contentlength64 > 0) && param->operation != 0x00000800 && res != 204 && res != 304)
		{
			do
			{
				if (param->chunked)
				{
					unsigned char smallbuf[32];
					while ((i = sockgetlinebuf(param, SERVER, smallbuf, 30, '\n', conf.timeouts[STRING_S])) == 2)
					{
						if (socksend(param->clisock, smallbuf, i, conf.timeouts[STRING_S]) != i)
						{
							{
								param->res = 533;
								goto CLEANRET;
							};
						}
						if (param->chunked == 2)
							break;
					}
					if (i < 3)
					{
						keepalive = 0;
						break;
					}
					if (socksend(param->clisock, smallbuf, i, conf.timeouts[STRING_S]) != i)
					{
						{
							param->res = 535;
							goto CLEANRET;
						};
					}
					if (param->chunked == 2)
					{
						if ((i = sockgetlinebuf(param, SERVER, smallbuf, 30, '\n', conf.timeouts[STRING_S])) != 2)
						{
							param->res = 534;
							goto CLEANRET;
						};
						if (socksend(param->clisock, smallbuf, i, conf.timeouts[STRING_S]) != i)
						{
							{
								param->res = 533;
								goto CLEANRET;
							};
						}
						break;
					}
					smallbuf[i] = 0;
					contentlength64 = 0;
					sscanf((char *)smallbuf, "%"
											 "I64"
											 "x",
						   &contentlength64);
					if (contentlength64 == 0)
					{
						param->chunked = 2;
					}
				}
				if (param->chunked != 2)
				{
					param->waitserver64 = contentlength64;
					if ((res = sockmap(param, conf.timeouts[CONNECTION_S], 0)) != 98)
					{
						{
							param->res = res;
							goto CLEANRET;
						};
					}
					param->waitserver64 = 0;
				}
			} while (param->chunked);
		}
		if (isconnect && res == 200 && param->operation)
		{
			{
				param->res = sockmap(param, conf.timeouts[CONNECTION_S], 0);
				goto CLEANRET;
			};
		}
		else if (isconnect)
		{
			ckeepalive = keepalive = 1;
		}
		else if (!hascontent && !param->chunked)
		{
			{
				param->res = sockmap(param, conf.timeouts[CONNECTION_S], 0);
				goto CLEANRET;
			};
		}
		contentlength64 = 0;
	REQUESTEND:
		if ((!ckeepalive || !keepalive) && param->remsock != (SOCKET)(~0))
		{
			so._shutdown(param->remsock, 0x02);
			so._closesocket(param->remsock);
			param->remsock = (SOCKET)(~0);
			{
				param->res = 0;
				goto CLEANRET;
			};
		}
		if (param->transparent && (!ckeepalive || !keepalive))
		{
			{
				param->res = 0;
				goto CLEANRET;
			};
		}
		logurl(param, (char *)buf, (char *)req, ftp);
		param->status = 0;
	}
CLEANRET:
	if (param->res != 555 && param->res && param->clisock != (SOCKET)(~0) && (param->res < 90 || param->res >= 800 || param->res == 100 || (param->res > 500 && param->res < 800)))
	{
		if ((param->res >= 509 && param->res < 517) || param->res > 900)
			while ((i = sockgetlinebuf(param, CLIENT, buf, (32768 * 2) - 1, '\n', conf.timeouts[STRING_S])) > 2)
				;
		if (param->res == 10)
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[2], (int)strlen(proxy_stringtable[2]), conf.timeouts[STRING_S]);
		}
		else if (res == 700 || res == 701)
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[16], (int)strlen(proxy_stringtable[16]), conf.timeouts[STRING_S]);
			socksend(param->clisock, (unsigned char *)ftpbuf, inftpbuf, conf.timeouts[STRING_S]);
		}
		else if (param->res == 100 || (param->res > 10 && param->res < 20) || (param->res > 701 && param->res <= 705))
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[1], (int)strlen(proxy_stringtable[1]), conf.timeouts[STRING_S]);
		}
		else if (param->res >= 20 && param->res < 30)
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[6], (int)strlen(proxy_stringtable[6]), conf.timeouts[STRING_S]);
		}
		else if (param->res >= 30 && param->res < 80)
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[5], (int)strlen(proxy_stringtable[5]), conf.timeouts[STRING_S]);
		}
		else if (param->res == 1 || (!param->srv->needuser && param->res < 10))
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[11], (int)strlen(proxy_stringtable[11]), conf.timeouts[STRING_S]);
		}
		else if (param->res < 10)
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[param->srv->usentlm ? 12 : 7], (int)strlen(proxy_stringtable[param->srv->usentlm ? 12 : 7]), conf.timeouts[STRING_S]);
		}
		else if (param->res == 999)
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[4], (int)strlen(proxy_stringtable[4]), conf.timeouts[STRING_S]);
		}
		else if (param->res == 519)
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[3], (int)strlen(proxy_stringtable[3]), conf.timeouts[STRING_S]);
		}
		else if (param->res == 517)
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[15], (int)strlen(proxy_stringtable[15]), conf.timeouts[STRING_S]);
		}
		else if (param->res == 780)
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[10], (int)strlen(proxy_stringtable[10]), conf.timeouts[STRING_S]);
		}
		else if (param->res >= 511 && param->res <= 516)
		{
			socksend(param->clisock, (unsigned char *)proxy_stringtable[0], (int)strlen(proxy_stringtable[0]), conf.timeouts[STRING_S]);
		}
	}
	logurl(param, (char *)buf, (char *)req, ftp);
	if (req)
		free(req);
	if (buf)
		free(buf);
	if (ftpbase)
		free(ftpbase);
	freeparam(param);
	return (((void *)0));
}
struct proxydef childdef = {
	proxychild,
	3128,
	0,
	S_PROXY,
	"-a - anonymous proxy\r\n"
	"-a1 - anonymous proxy with random client IP spoofing\r\n"};
DWORD __stdcall threadfunc(LPVOID p)
{
	int i = -1;
	if (((struct clientparam *)p)->srv->cbsock != (SOCKET)(~0))
	{
		int size = sizeof(((struct clientparam *)p)->sinsr);
		struct pollfd fds;
		fds.fd = ((struct clientparam *)p)->srv->cbsock;
		fds.events = (0x0100 | 0x0200);
		fds.revents = 0;
		for (i = 5 + (((struct clientparam *)p)->srv->maxchild >> 10); i; i--)
		{
			if (so._poll(&fds, 1, 1000 * CONNBACK_TO) != 1)
			{
				dolog(((struct clientparam *)p), (unsigned char *)"Connect back not received, check connback client");
				i = 0;
				break;
			}
			((struct clientparam *)p)->remsock = so._accept(((struct clientparam *)p)->srv->cbsock, (struct sockaddr *)&((struct clientparam *)p)->sinsr, &size);
			if (((struct clientparam *)p)->remsock == (SOCKET)(~0))
			{
				dolog(((struct clientparam *)p), (unsigned char *)"Connect back accept() failed");
				continue;
			}
			{
				unsigned long ul = 1;
				ioctlsocket(((struct clientparam *)p)->remsock, (0x80000000 | (((long)sizeof(u_long) & 0x7f) << 16) | (('f') << 8) | (126)), &ul);
			}
			if (socksendto(((struct clientparam *)p)->remsock, (struct sockaddr *)&((struct clientparam *)p)->sinsr, (unsigned char *)"C", 1, CONNBACK_TO * 1000) != 1)
			{
				dolog(((struct clientparam *)p), (unsigned char *)"Connect back sending command failed");
				so._closesocket(((struct clientparam *)p)->remsock);
				((struct clientparam *)p)->remsock = (SOCKET)(~0);
				continue;
			}
			break;
		}
	}
	if (!i)
	{
		((struct clientparam *)p)->res = 13;
		freeparam(((struct clientparam *)p));
	}
	else
	{
		((struct clientparam *)p)->srv->pf((struct clientparam *)p);
	}
	return 0;
}
struct socketoptions sockopts[] = {
	{0x0001, "TCP_NODELAY"},
	{10, "TCP_TIMESTAMPS"},
	{0x0004, "SO_REUSEADDR"},
	{0x3006, "SO_PORT_SCALABILITY"},
	{0x3007, "SO_REUSE_UNICASTPORT"},
	{0x0008, "SO_KEEPALIVE"},
	{0x0010, "SO_DONTROUTE"},
	{15, "TCP_FASTOPEN"},
	{0, ((void *)0)}};
char optsbuf[1024];
char *printopts(char *sep)
{
	int i = 0, pos = 0;
	for (; sockopts[i].optname; i++)
		pos += sprintf(optsbuf + pos, "%s%s", i ? sep : "", sockopts[i].optname);
	return optsbuf;
}
int getopts(const char *s)
{
	int i = 0, ret = 0;
	for (; sockopts[i].optname; i++)
		if (strstr(s, sockopts[i].optname))
			ret |= (1 << i);
	return ret;
}
void setopts(SOCKET s, int opts)
{
	int i, opt, set;
	for (i = 0; opts >= (opt = (1 << i)); i++)
	{
		set = 1;
		if (opts & opt)
			setsockopt(s, *sockopts[i].optname == 'T' ? IPPROTO_TCP : *sockopts[i].optname == 'I' ? (0xffff - 4)
																								  : 0xffff,
					   sockopts[i].opt, (char *)&set, sizeof(set));
	}
}
int main(int argc, char **argv)
{
	SOCKET sock = (SOCKET)(~0), new_sock = (SOCKET)(~0);
	int i = 0;
	int size;
	unsigned thread;
	struct clientparam defparam;
	struct srvparam srv;
	struct clientparam *newparam;
	int error = 0;
	unsigned sleeptime;
	unsigned char buf[256];
	char *hostname = ((void *)0);
	int opt = 1, isudp = 0, iscbl = 0, iscbc = 0;
	unsigned char *cbc_string = ((void *)0), *cbl_string = ((void *)0);
	struct sockaddr_in6 cbsa;
	FILE *fp = ((void *)0);
	struct linger lg;
	int nlog = 5000;
	char loghelp[] =
		" -d go to background (daemon)\n"
		"-g(GRACE_TRAFF,GRACE_NUM,GRACE_DELAY) - delay GRACE_DELAY milliseconds before polling if average polling size below  GRACE_TRAFF bytes and GRACE_NUM read operations in single directions are detected within 1 second to minimize polling\n"
		" -fFORMAT logging format (see documentation)\n"
		" -l log to stderr\n"
		" -lFILENAME log to FILENAME\n"
		" -b(BUFSIZE) size of network buffer (default 4096 for TCP, 16384 for UDP)\n"
		" -S(STACKSIZE) value to add to default client thread stack size\n"
		" -t be silent (do not log service start/stop)\n"
		"\n"
		" -iIP ip address or internal interface (clients are expected to connect)\n"
		" -eIP ip address or external interface (outgoing connection will have this)\n"
		" -rHOST:PORT Use IP:port for connect back proxy instead of listen port\n"
		" -RHOST:PORT Use PORT to listen connect back proxy connection to pass data to\n"
		" -4 Use IPv4 for outgoing connections\n"
		" -6 Use IPv6 for outgoing connections\n"
		" -46 Prefer IPv4 for outgoing connections, use both IPv4 and IPv6\n"
		" -64 Prefer IPv6 for outgoing connections, use both IPv4 and IPv6\n"
		" -ocOPTIONS, -osOPTIONS, -olOPTIONS, -orOPTIONS -oROPTIONS - options for\n"
		" to-client (oc), to-server (os), listening (ol) socket, connect back client\n"
		" (or) socket, connect back server (oR) listening socket\n"
		" where possible options are: ";
	unsigned long ul = 1;
	HANDLE h;
	WSADATA wd;
	WSAStartup(((WORD)(((BYTE)(((DWORD_PTR)(1)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(1)) & 0xff))) << 8)), &wd);
	srvinit(&srv, &defparam);
	srv.pf = childdef.pf;
	isudp = childdef.isudp;
	srv.service = defparam.service = childdef.service;
	srv.needuser = 0;
	InitializeCriticalSection(&log_mutex);
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] == '-')
		{
			switch (argv[i][1])
			{
			case 'd':
				if (!conf.demon)
					FreeConsole();
				conf.demon = 1;
				break;
			case 'l':
				srv.logfunc = logstdout;
				if (srv.logtarget)
					free(srv.logtarget);
				srv.logtarget = (unsigned char *)strdup(argv[i] + 2);
				if (argv[i][2])
				{
					if (argv[i][2] == '@')
					{
					}
					else
					{
						fp = fopen(argv[i] + 2, "a");
						if (fp)
						{
							srv.stdlog = fp;
						}
					}
				}
				break;
			case 'i':
				getip46(46, (unsigned char *)argv[i] + 2, (struct sockaddr *)&srv.intsa);
				break;
			case 'e':
			{
				struct sockaddr_in6 sa6;
				memset(&sa6, 0, sizeof(sa6));
				error = !getip46(46, (unsigned char *)argv[i] + 2, (struct sockaddr *)&sa6);
				if (!error)
				{
					if (*(&(((struct sockaddr_in *)&sa6)->sin_family)) == 2)
						srv.extsa = sa6;
					else
						srv.extsa6 = sa6;
				}
			}
			break;
			case 'N':
				getip46(46, (unsigned char *)argv[i] + 2, (struct sockaddr *)&srv.extNat);
				break;
			case 'p':
				*(((struct sockaddr_in *)&srv.intsa)->sin_family == 23 ? &((struct sockaddr_in6 *)&srv.intsa)->sin6_port : &((struct sockaddr_in *)&srv.intsa)->sin_port) = htons(atoi(argv[i] + 2));
				break;
			case '4':
			case '6':
				srv.family = atoi(argv[i] + 1);
				break;
			case 'b':
				srv.bufsize = atoi(argv[i] + 2);
				break;
			case 'n':
				srv.usentlm = atoi(argv[i] + 2);
				break;
			case 'f':
				if (srv.logformat)
					free(srv.logformat);
				srv.logformat = (unsigned char *)strdup(argv[i] + 2);
				break;
			case 't':
				srv.silent = 1;
				break;
			case 'h':
				hostname = argv[i] + 2;
				break;
			case 'r':
				cbc_string = (unsigned char *)strdup(argv[i] + 2);
				iscbc = 1;
				break;
			case 'R':
				cbl_string = (unsigned char *)strdup(argv[i] + 2);
				iscbl = 1;
				break;
			case 'u':
				srv.needuser = 0;
				if (*(argv[i] + 2))
					srv.needuser = atoi(argv[i] + 2);
				break;
			case 'T':
				srv.transparent = 1;
				break;
			case 'S':
				srv.stacksize = atoi(argv[i] + 2);
				break;
			case 'a':
				srv.anonymous = 1 + atoi(argv[i] + 2);
				break;
			case 'g':
				sscanf(argv[i] + 2, "%d,%d,%d", &srv.gracetraf, &srv.gracenum, &srv.gracedelay);
				break;
			case 's':
				srv.singlepacket = 1 + atoi(argv[i] + 2);
				break;
			case 'o':
				switch (argv[i][2])
				{
				case 's':
					srv.srvsockopts = getopts(argv[i] + 3);
					break;
				case 'c':
					srv.clisockopts = getopts(argv[i] + 3);
					break;
				case 'l':
					srv.lissockopts = getopts(argv[i] + 3);
					break;
				case 'r':
					srv.cbcsockopts = getopts(argv[i] + 3);
					break;
				case 'R':
					srv.cbcsockopts = getopts(argv[i] + 3);
					break;
				default:
					error = 1;
				}
				if (!error)
					break;
			default:
				error = 1;
				break;
			}
		}
		else
			break;
	}
	if (error || i != argc)
	{
		fprintf((__acrt_iob_func(2)), "%s of %s\n"
									  "Usage: %s options\n"
									  "Available options are:\n"
									  "%s\n"
									  "\t%s\n"
									  " -pPORT - service port to accept connections\n"
									  "%s"
									  "\tExample: %s -i127.0.0.1\n\n"
									  "%s",
				argv[0],
				conf.stringtable ? (char *)conf.stringtable[3] : "3proxy-0.9.4"
																 " ("
																 ""
																 ")",
				argv[0], loghelp, printopts("\n\t"), childdef.helpmessage, argv[0],
				copyright);
		return (1);
	}
	srvinit2(&srv, &defparam);
	if (!*(&(((struct sockaddr_in *)&srv.intsa)->sin_family)))
		*(&(((struct sockaddr_in *)&srv.intsa)->sin_family)) = 2;
	if (!*(((struct sockaddr_in *)&srv.intsa)->sin_family == 23 ? &((struct sockaddr_in6 *)&srv.intsa)->sin6_port : &((struct sockaddr_in *)&srv.intsa)->sin_port))
		*(((struct sockaddr_in *)&srv.intsa)->sin_family == 23 ? &((struct sockaddr_in6 *)&srv.intsa)->sin6_port : &((struct sockaddr_in *)&srv.intsa)->sin_port) = htons(childdef.port);
	*(&(((struct sockaddr_in *)&srv.extsa)->sin_family)) = 2;
	*(&(((struct sockaddr_in *)&srv.extsa6)->sin_family)) = 23;
	if (hostname)
		parsehostname(hostname, &defparam, childdef.port);
	if (!iscbc)
	{
		if (srv.srvsock == (SOCKET)(~0))
		{
			if (!isudp)
			{
				sock = so._socket((((struct sockaddr_in *)&srv.intsa)->sin_family == 23 ? 23 : 2), 1, IPPROTO_TCP);
			}
			else
			{
				sock = so._socket((((struct sockaddr_in *)&srv.intsa)->sin_family == 23 ? 23 : 2), 2, IPPROTO_UDP);
			}
			if (sock == (SOCKET)(~0))
			{
				perror("socket()");
				return -2;
			}
			setopts(sock, srv.lissockopts);
			ioctlsocket(sock, (0x80000000 | (((long)sizeof(u_long) & 0x7f) << 16) | (('f') << 8) | (126)), &ul);
			srv.srvsock = sock;
			opt = 1;
			if (so._setsockopt(sock, 0xffff, 0x0004, (char *)&opt, sizeof(int)))
				perror("setsockopt()");
		}
		size = sizeof(srv.intsa);
		for (sleeptime = 1 * 100; so._bind(sock, (struct sockaddr *)&srv.intsa, (((struct sockaddr_in *)&srv.intsa)->sin_family == 23 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))) == -1; Sleep(sleeptime))
		{
			sprintf((char *)buf, "bind(): %s", strerror(WSAGetLastError()));
			if (!srv.silent)
				dolog(&defparam, buf);
			sleeptime = (sleeptime << 1);
			if (!sleeptime)
			{
				so._closesocket(sock);
				return -3;
			}
		}
		if (!isudp)
		{
			if (so._listen(sock, srv.backlog ? srv.backlog : 1 + (srv.maxchild >> 3)) == -1)
			{
				sprintf((char *)buf, "listen(): %s", strerror(WSAGetLastError()));
				if (!srv.silent)
					dolog(&defparam, buf);
				return -4;
			}
		}
		else
			defparam.clisock = sock;
		if (!srv.silent && !iscbc)
		{
			sprintf((char *)buf, "Accepting connections [%u/%u]", (unsigned)GetCurrentProcessId(), (unsigned)GetCurrentThreadId());
			dolog(&defparam, buf);
		}
	}
	if (iscbl)
	{
		parsehost(srv.family, cbl_string, (struct sockaddr *)&cbsa);
		if ((srv.cbsock = so._socket((((struct sockaddr_in *)&cbsa)->sin_family == 23 ? 23 : 2), 1, IPPROTO_TCP)) == (SOCKET)(~0))
		{
			dolog(&defparam, (unsigned char *)"Failed to allocate connect back socket");
			return -6;
		}
		opt = 1;
		so._setsockopt(srv.cbsock, 0xffff, 0x0004, (char *)&opt, sizeof(int));
		setopts(srv.cbsock, srv.cbssockopts);
		if (so._bind(srv.cbsock, (struct sockaddr *)&cbsa, (((struct sockaddr_in *)&cbsa)->sin_family == 23 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))) == -1)
		{
			dolog(&defparam, (unsigned char *)"Failed to bind connect back socket");
			return -7;
		}
		if (so._listen(srv.cbsock, 1 + (srv.maxchild >> 4)) == -1)
		{
			dolog(&defparam, (unsigned char *)"Failed to listen connect back socket");
			return -8;
		}
	}
	srv.fds.fd = sock;
	srv.fds.events = (0x0100 | 0x0200);
	for (;;)
	{
		for (;;)
		{
			while ((conf.paused == srv.paused && srv.childcount >= srv.maxchild))
			{
				nlog++;
				if (!srv.silent && nlog > 5000)
				{
					sprintf((char *)buf, "Warning: too many connected clients (%d/%d)", srv.childcount, srv.maxchild);
					dolog(&defparam, buf);
					nlog = 0;
				}
				Sleep(1);
			}
			if (iscbc)
				break;
			if (conf.paused != srv.paused)
				break;
			if (srv.fds.events & (0x0100 | 0x0200))
			{
				error = so._poll(&srv.fds, 1, 1000);
			}
			else
			{
				Sleep(1);
				continue;
			}
			if (error >= 1)
				break;
			if (error == 0)
				continue;
			if (WSAGetLastError() != 10035L && WSAGetLastError() != 10035L)
			{
				sprintf((char *)buf, "poll(): %s/%d", strerror(WSAGetLastError()), WSAGetLastError());
				if (!srv.silent)
					dolog(&defparam, buf);
				break;
			}
		}
		if ((conf.paused != srv.paused) || (error < 0))
			break;
		error = 0;
		if (!isudp)
		{
			size = sizeof(defparam.sincr);
			if (iscbc)
			{
				new_sock = so._socket((((struct sockaddr_in *)&defparam.sincr)->sin_family == 23 ? 23 : 2), 1, IPPROTO_TCP);
				if (new_sock != (SOCKET)(~0))
				{
					setopts(new_sock, srv.cbcsockopts);
					parsehost(srv.family, cbc_string, (struct sockaddr *)&defparam.sincr);
					if (connectwithpoll(new_sock, (struct sockaddr *)&defparam.sincr, (((struct sockaddr_in *)&defparam.sincr)->sin_family == 23 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)), CONNBACK_TO))
					{
						so._closesocket(new_sock);
						new_sock = (SOCKET)(~0);
						Sleep(1);
						continue;
					}
					if (sockrecvfrom(new_sock, (struct sockaddr *)&defparam.sincr, buf, 1, 60 * 1000) != 1 || *buf != 'C')
					{
						so._closesocket(new_sock);
						new_sock = (SOCKET)(~0);
						Sleep(1);
						continue;
					}
				}
				else
				{
					Sleep(1);
					continue;
				}
			}
			else
			{
				new_sock = so._accept(sock, (struct sockaddr *)&defparam.sincr, &size);
				if (new_sock == (SOCKET)(~0))
				{
					switch (WSAGetLastError())
					{
					case 10024L:
					case 10055L:
					case 10050L:
						Sleep(1 * 10);
						break;
					case 10004L:
						error = 1;
						break;
					default:
						break;
					}
					nlog++;
					if (!srv.silent && (error || nlog > 5000))
					{
						sprintf((char *)buf, "accept(): %s", strerror(WSAGetLastError()));
						dolog(&defparam, buf);
						nlog = 0;
					}
					continue;
				}
				setopts(new_sock, srv.clisockopts);
			}
			size = sizeof(defparam.sincl);
			if (so._getsockname(new_sock, (struct sockaddr *)&defparam.sincl, &size))
			{
				sprintf((char *)buf, "getsockname(): %s", strerror(WSAGetLastError()));
				if (!srv.silent)
					dolog(&defparam, buf);
				continue;
			}
			ioctlsocket(new_sock, (0x80000000 | (((long)sizeof(u_long) & 0x7f) << 16) | (('f') << 8) | (126)), &ul);
			lg.l_onoff = 1;
			lg.l_linger = conf.timeouts[STRING_L];
			so._setsockopt(new_sock, 0xffff, 0x0080, (char *)&lg, sizeof(lg));
			so._setsockopt(new_sock, 0xffff, 0x0100, (char *)&opt, sizeof(int));
		}
		else
		{
			srv.fds.events = 0;
		}
		if (!(newparam = malloc(sizeof(defparam))))
		{
			if (!isudp)
				so._closesocket(new_sock);
			defparam.res = 21;
			if (!srv.silent)
				dolog(&defparam, (unsigned char *)"Memory Allocation Failed");
			Sleep(1);
			continue;
		};
		*newparam = defparam;
		if (defparam.hostname)
			newparam->hostname = (unsigned char *)strdup((char *)defparam.hostname);
		clearstat(newparam);
		if (!isudp)
			newparam->clisock = new_sock;
		newparam->prev = newparam->next = ((void *)0);
		error = 0;
		EnterCriticalSection(&srv.counter_mutex);
		if (!srv.child)
		{
			srv.child = newparam;
		}
		else
		{
			newparam->next = srv.child;
			srv.child = srv.child->prev = newparam;
		}
		h = (HANDLE)_beginthreadex((LPSECURITY_ATTRIBUTES)((void *)0), (unsigned)(16384 + srv.stacksize), (void *)threadfunc, (void *)newparam, 0, &thread);
		srv.childcount++;
		if (h)
		{
			newparam->threadid = (unsigned)thread;
			CloseHandle(h);
		}
		else
		{
			sprintf((char *)buf, "_beginthreadex(): %s", _strerror(((void *)0)));
			if (!srv.silent)
				dolog(&defparam, buf);
			error = 1;
		}
		LeaveCriticalSection(&srv.counter_mutex);
		if (error)
			freeparam(newparam);
		memset(&defparam.sincl, 0, sizeof(defparam.sincl));
		memset(&defparam.sincr, 0, sizeof(defparam.sincr));
		if (isudp)
			while (!srv.fds.events)
				Sleep(1);
	}
	if (!srv.silent)
		srv.logfunc(&defparam, (unsigned char *)"Exiting thread");
	srvfree(&srv);
	if (defparam.hostname)
		free(defparam.hostname);
	if (cbc_string)
		free(cbc_string);
	if (cbl_string)
		free(cbl_string);
	if (fp)
		fclose(fp);
	return 0;
}
void srvinit(struct srvparam *srv, struct clientparam *param)
{
	memset(srv, 0, sizeof(struct srvparam));
	srv->version = conf.version + 1;
	srv->paused = conf.paused;
	srv->logfunc = havelog ? conf.logfunc : lognone;
	srv->noforce = conf.noforce;
	srv->logformat = conf.logformat ? (unsigned char *)strdup((char *)conf.logformat) : ((void *)0);
	srv->authfunc = conf.authfunc;
	srv->usentlm = 0;
	srv->maxchild = conf.maxchild;
	srv->backlog = conf.backlog;
	srv->stacksize = conf.stacksize;
	srv->time_start = time(((void *)0));
	if (havelog && conf.logtarget)
	{
		srv->logtarget = (unsigned char *)strdup((char *)conf.logtarget);
	}
	srv->srvsock = (SOCKET)(~0);
	srv->logdumpsrv = conf.logdumpsrv;
	srv->logdumpcli = conf.logdumpcli;
	srv->cbsock = (SOCKET)(~0);
	srv->needuser = 1;
	memset(param, 0, sizeof(struct clientparam));
	param->srv = srv;
	param->version = srv->version;
	param->paused = srv->paused;
	param->remsock = param->clisock = param->ctrlsock = param->ctrlsocksrv = (SOCKET)(~0);
	*(&(((struct sockaddr_in *)&param->req)->sin_family)) = *(&(((struct sockaddr_in *)&param->sinsl)->sin_family)) = *(&(((struct sockaddr_in *)&param->sinsr)->sin_family)) = *(&(((struct sockaddr_in *)&param->sincr)->sin_family)) = *(&(((struct sockaddr_in *)&param->sincl)->sin_family)) = 2;
	InitializeCriticalSection(&srv->counter_mutex);
	srv->intsa = conf.intsa;
	srv->extsa = conf.extsa;
	srv->extsa6 = conf.extsa6;
}
void srvinit2(struct srvparam *srv, struct clientparam *param)
{
	if (srv->logformat)
	{
		char *s;
		if (*srv->logformat == '-' && (s = strchr((char *)srv->logformat + 1, '+')) && s[1])
		{
			unsigned char *logformat = srv->logformat;
			*s = 0;
			srv->nonprintable = (unsigned char *)strdup((char *)srv->logformat + 1);
			srv->replace = s[1];
			srv->logformat = (unsigned char *)strdup(s + 2);
			*s = '+';
			free(logformat);
		}
	}
	memset(&param->sinsl, 0, sizeof(param->sinsl));
	memset(&param->sinsr, 0, sizeof(param->sinsr));
	memset(&param->req, 0, sizeof(param->req));
	*(&(((struct sockaddr_in *)&param->sinsl)->sin_family)) = 2;
	*(&(((struct sockaddr_in *)&param->sinsr)->sin_family)) = 2;
	*(&(((struct sockaddr_in *)&param->req)->sin_family)) = 2;
	param->sincr = param->sincl = srv->intsa;
	if (srv->family == 6 || srv->family == 64)
		param->sinsr = srv->extsa6;
	else
		param->sinsr = srv->extsa;
}
void srvfree(struct srvparam *srv)
{
	if (srv->srvsock != (SOCKET)(~0))
		so._closesocket(srv->srvsock);
	srv->srvsock = (SOCKET)(~0);
	if (srv->cbsock != (SOCKET)(~0))
		so._closesocket(srv->cbsock);
	srv->cbsock = (SOCKET)(~0);
	srv->service = S_ZOMBIE;
	while (srv->child)
		Sleep(1 * 100);
	DeleteCriticalSection(&srv->counter_mutex);
	if (srv->target)
		free(srv->target);
	if (srv->logtarget)
		free(srv->logtarget);
	if (srv->logformat)
		free(srv->logformat);
	if (srv->nonprintable)
		free(srv->nonprintable);
}
void freeparam(struct clientparam *param)
{
	if (param->res == 2)
		return;
	if (param->datfilterssrv)
		free(param->datfilterssrv);
	if (param->clibuf)
		free(param->clibuf);
	if (param->srvbuf)
		free(param->srvbuf);
	if (param->srv)
	{
		EnterCriticalSection(&param->srv->counter_mutex);
		if (param->prev)
		{
			param->prev->next = param->next;
		}
		else
			param->srv->child = param->next;
		if (param->next)
		{
			param->next->prev = param->prev;
		}
		(param->srv->childcount)--;
		LeaveCriticalSection(&param->srv->counter_mutex);
	}
	if (param->hostname)
		free(param->hostname);
	if (param->username)
		free(param->username);
	if (param->password)
		free(param->password);
	if (param->extusername)
		free(param->extusername);
	if (param->extpassword)
		free(param->extpassword);
	if (param->ctrlsocksrv != (SOCKET)(~0) && param->ctrlsocksrv != param->remsock)
	{
		so._shutdown(param->ctrlsocksrv, 0x02);
		so._closesocket(param->ctrlsocksrv);
	}
	if (param->ctrlsock != (SOCKET)(~0) && param->ctrlsock != param->clisock)
	{
		so._shutdown(param->ctrlsock, 0x02);
		so._closesocket(param->ctrlsock);
	}
	if (param->remsock != (SOCKET)(~0))
	{
		so._shutdown(param->remsock, 0x02);
		so._closesocket(param->remsock);
	}
	if (param->clisock != (SOCKET)(~0))
	{
		so._shutdown(param->clisock, 0x02);
		so._closesocket(param->clisock);
	}
	free(param);
}
FILTER_ACTION handlepredatflt(struct clientparam *cparam)
{
	return PASS;
}
FILTER_ACTION handledatfltcli(struct clientparam *cparam, unsigned char **buf_p, int *bufsize_p, int offset, int *length_p)
{
	return PASS;
}
FILTER_ACTION handledatfltsrv(struct clientparam *cparam, unsigned char **buf_p, int *bufsize_p, int offset, int *length_p)
{
	FILTER_ACTION action;
	int i;
	for (i = 0; i < cparam->ndatfilterssrv; i++)
	{
		action = (*cparam->datfilterssrv[i]->filter->filter_data_srv)(cparam->datfilterssrv[i]->data, cparam, buf_p, bufsize_p, offset, length_p);
		if (action != CONTINUE)
			return action;
	}
	return PASS;
}