/*************************************************
 * server.c                                    		  *
 *   create the server window    		  *
 *   send message to client          		  *
 *************************************************/
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define _WIN32_ 1

#ifdef _WIN32_
#include <windows.h>
#endif

#ifdef _LINUX_
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <semaphore.h>
#endif
#define bool _Bool
#define true 1
#define false 0
#define MAXSIZE 1024
bool socket_start;
GtkTextBuffer *show_buffer,*input_buffer;
void quit_win(GtkWidget *,gpointer);

gboolean from_host;//��ʾ�ͻ����Ƿ��Ѿ��򱾷�������������Ϣ
GSocket *sock;
GSocket *cl_sock;
int issucceed=-1;
int cl_sockfd;
struct sockaddr_in saddr,caddr;//saddr��ʾ���ط�������ַ��Ϣ��caddr���ڱ����򱾷�����������Ϣ��������ַ��Ϣ

/* show errors such as "no input","haven't create sockt" etc. */
void show_err(char *err)
{
	GtkTextIter start,end;
	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(show_buffer),&start,&end);/*��û�������ʼ�ͽ���λ�õ�Iter*/
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(show_buffer),&end,err,strlen(err));
}

/* show the received message */
void show_remote_text(char rcvd_mess[])
{
	GtkTextIter start,end;
	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(show_buffer),&start,&end);/*��û�������ʼ�ͽ���λ�õ�Iter*/
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(show_buffer),&end,"Client:\n",8);/*�����ı���������*/
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(show_buffer),&end,rcvd_mess,strlen(rcvd_mess));/*�����ı���������*/
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(show_buffer),&end,"\n",1);/*���뻻�е�������*/
}

/* Receive function */
gpointer recv_func(gpointer arg)/*recv_func(void *arg)*/
{
	 char rcvd_mess[MAXSIZE];
	 GInputVector vector;
	 GError *error = NULL;
	 vector.buffer = rcvd_mess;
	 vector.size = MAXSIZE;
	 while(1)
	 {
		memset(rcvd_mess,0,MAXSIZE);
		if(g_socket_receive(cl_sock,vector.buffer, vector.size,NULL, &error)<0)
		{
			perror("server recv error\n");
			exit(1);
		}
	    g_print("Messages = %s\n", rcvd_mess);
	    show_remote_text(rcvd_mess);
	    g_print("Waiting����");
	 }
}

gint build_socket(void *arg)
{
    g_type_init();
    GInetAddress *iface_address = g_inet_address_new_from_string ("127.0.0.1");
    GSocketAddress *connect_address = g_inet_socket_address_new (iface_address, 8500);
    GError *err = NULL;
    sock = g_socket_new(G_SOCKET_FAMILY_IPV4,
    					G_SOCKET_TYPE_STREAM,
						G_SOCKET_PROTOCOL_TCP,
                        &err);
    g_assert(err == NULL);

	//(bind(sock,(struct sockaddr *)&saddr,sizeof(struct sockaddr_in))==-1)
	if(g_socket_bind (sock,
			   	   	  connect_address,
					  TRUE,
					  &err)!=TRUE)
	{
		show_err("Bind Error");
		exit(1);
	}
	if(g_socket_listen (sock, &err)!=TRUE)
	{
		show_err("Listen Error");
		exit(1);
	}
	while(1)
	{
			cl_sock=g_socket_accept (sock,NULL,&err);
               /* Create a sub thread,call recv_func() */
			g_thread_new("recv_func",recv_func, cl_sock);
			g_print("recv_func start...\n");
			return 0;
		    from_host=true;
    }
}

/* Send function,send message to client */
void send_func(const char *text)
{
	int n;
	GError *err = NULL;
	n=g_socket_send(cl_sock,
	               text,
			       MAXSIZE,
	               NULL,
	               &err);
	if(n<0)
	{
		perror("S send error\n");
		exit(1);
	}
}

/* show the input text */
void show_local_text(const gchar* text)
{
	GtkTextIter start,end;
	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(show_buffer),&start,&end);/*��û�������ʼ�ͽ���λ�õ�Iter*/
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(show_buffer),&end,"Me:\n",4);/*�����ı���������*/
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(show_buffer),&end,text,strlen(text));/*�����ı���������*/
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(show_buffer),&end,"\n",1);/*�����ı���������*/
}
/* clean the input text */
void clean_send_text()
{
	GtkTextIter start,end;
	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(input_buffer),&start,&end);/*��û�������ʼ�ͽ���λ�õ�Iter*/
	gtk_text_buffer_delete(GTK_TEXT_BUFFER(input_buffer),&start,&end);/*���뵽������*/
}
/* get the input text,and send it */
void send_text()
{
	GtkTextIter start,end;
	gchar *text;
	 /* Socket creating has succeed ,so send message */
	text=(gchar *)malloc(MAXSIZE);
	if(text==NULL)
	{
		printf("Malloc error!\n");
		exit(1);
	}
	/* get text */
	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(input_buffer),&start,&end);
	text=gtk_text_buffer_get_text(GTK_TEXT_BUFFER(input_buffer),&start,&end,FALSE);
	/* If there is no input,do nothing but return */
	if(strcmp(text,"")!=0)
	{
		send_func(text);
		clean_send_text();
		show_local_text(text);
	}
	else
		show_err("The message can not be empty ...\n");
	free(text);
}
void startup(void)
{
        //res=pthread_create(&listen_thread,NULL,build_socket,NULL);
    g_thread_new("build_socket",build_socket, NULL);
    //res=build_socket(sock);
	//if(res!=0)
	//{
    //        show_err("socket build error");
    //        exit(EXIT_FAILURE);
	//}
	show_err("The server has been started !\n");
    return;
}

/* quit */
void quit_win(GtkWidget *window,gpointer data)
{
	gtk_main_quit();
}

int main(int argc,char **argv)
{
	GtkWidget *window;
	GtkWidget *show_text,*input_text;
	GtkWidget *send_button,*quit_button,*cls_button,*start_button;
	GtkWidget *hbox,*vbox;
	GtkWidget *scrolled1,*scrolled2;
	GtkWidget *space_label;
	gtk_init(&argc,&argv);
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window),"Server");
	gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window),430,320);
	/* "quit" button */
	g_signal_connect(G_OBJECT(window),"destroy",G_CALLBACK(quit_win),NULL);

	space_label=gtk_label_new("                          ");
	/* set button */
	send_button=gtk_button_new_with_label("Send");
	quit_button=gtk_button_new_with_label("Close");
	cls_button=gtk_button_new_with_label("Clear");
	start_button=gtk_button_new_with_label("Startup");
	/* set textbox */
	show_text=gtk_text_view_new();
	input_text=gtk_text_view_new();
	/* get the buffer of textbox */
	show_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(show_text));
	input_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(input_text));
	/* set textbox to diseditable */
	gtk_text_view_set_editable(GTK_TEXT_VIEW(show_text),FALSE);
	/* scroll window */
	scrolled1=gtk_scrolled_window_new(NULL,NULL);
	scrolled2=gtk_scrolled_window_new(NULL,NULL);
	/* create a textbox */
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled1),show_text);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled2),input_text);
	/* setting of window */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled1),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled2),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	hbox=gtk_box_new (GTK_ORIENTATION_HORIZONTAL,3);//gtk_hbox_new(FALSE,2);
	vbox=gtk_box_new (GTK_ORIENTATION_VERTICAL,3);//gtk_vbox_new(FALSE,2);
	/* click quit to call quit_win*/
	g_signal_connect(G_OBJECT(quit_button),"clicked",G_CALLBACK(quit_win),NULL);
	/* click to clear input screen*/
	g_signal_connect(G_OBJECT(cls_button),"clicked",G_CALLBACK(clean_send_text),NULL);
	/* click Start to build socket*/
	g_signal_connect(G_OBJECT(start_button),"clicked",G_CALLBACK(startup),NULL);
	/* create window */
	gtk_box_pack_start(GTK_BOX(hbox),start_button,FALSE,FALSE,2);
	gtk_box_pack_start(GTK_BOX(hbox),space_label,TRUE,TRUE,2);
	gtk_box_pack_start(GTK_BOX(hbox),send_button,FALSE,FALSE,2);
	gtk_box_pack_start(GTK_BOX(hbox),cls_button,FALSE,FALSE,2);
	gtk_box_pack_start(GTK_BOX(hbox),quit_button,FALSE,FALSE,2);
	gtk_box_pack_start(GTK_BOX(vbox),scrolled1,TRUE,TRUE,2);
	gtk_box_pack_start(GTK_BOX(vbox),scrolled2,TRUE,TRUE,2);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,2);
	gtk_container_add(GTK_CONTAINER(window),vbox);
	/* click send button ,then call send_text*/
	g_signal_connect(G_OBJECT(send_button),"clicked",G_CALLBACK(send_text),NULL);
	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}
