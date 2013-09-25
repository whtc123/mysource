
void (*PlugCmdCallback)(void *response,void *arg);


typedef struct PluginObject
{
	int MIME;
	int source;
	//int sink;
	char *name;
	int plugin_cmd(cJSON *cmd,PlugCmdCallback callback,void *arg);
	int set_sink_callback();
	queue_t node; 
} PluginObject;

typedef struct Tunnel
{
	unsigned int max_cnt;
	unsigned int cnt;
	queue_t buff_queue;
}Tunnel;

typedef struct TunnelEnd
{
	Tunnel *tunnel;
	Buff *cur_buf;
}TunnelEnd;

TunnelEnd *tunnelend_new_r(Tunnel *tunnel);
TunnelEnd *tunnelend_new_w(Tunnel *tunnel);

void tunnelend_delete(Tunnel *tunnel,TunnelEnd *end);



int plugin_mng_connect(PluginObject *source,PluginObject *sink);
int plugin_mng_disconnect(PluginObject *source,PluginObject *sink);
PluginObject *plugin_mng_create(char *so_path,cJSON *cmd);
int plugin_mng_destory(PluginObject *obj);

