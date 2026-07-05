#ifndef WEB_API_CGI_H
#define WEB_API_CGI_H

struct cgi_para;
struct connstruct;

void web_api_cgi_bind_path(const char *path);
int web_api_cgi_handler(struct cgi_para *para);
int web_ui_index_serve(struct connstruct *cn);

#endif /* WEB_API_CGI_H */
