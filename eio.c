// gcc -o eio_file_ls eio.c `pkg-config --cflags --libs ecore eio eina` -g
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Eio.h>
#include <Ecore.h>

static Eina_List * processes;

typedef struct process process_t;
struct process
{
   int pid;
   const char * name;
   double mem;
   double proc;
};

process_t get_process_information(const char * process);

static Eina_Bool
_filter_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, const char *file)
{
   char *last_slash = strrchr(file, '/');
   return (last_slash[1] > '9') ? EINA_FALSE : EINA_TRUE;
}

static void
_main_cb(void *data, Eio_File *handler EINA_UNUSED, const char *file)
{
   process_t p ;
   int *number_of_listed_files = (int *)data;

   p = get_process_information(file);
   (*number_of_listed_files)++;
}

static void
_done_cb(void *data, Eio_File *handler EINA_UNUSED)
{
   int *number_of_listed_files = (int *)data;
   printf("Number of listed files:%d\n" \
         "ls operation is done, quitting.\n", *number_of_listed_files);
   ecore_main_loop_quit();
}

static void
_error_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, int error)
{
   fprintf(stderr, "Something has gone wrong:%s\n", strerror(error));
   ecore_main_loop_quit();
}

int
main(int argc, char **argv)
{
   int number_of_listed_files = 0;
   process_t p = { 1, "p", 50.5, 43.3 };

   printf("Playing with process %s\n", p.name);

   ecore_init();
   eio_init();
   eina_init();

   processes = NULL;
   /*printf("processes count: %d\n", eina_list_count(processes));*/
   /*processes = eina_list_append(processes,  &p);*/
   /*printf("processes count: %d\n", eina_list_count(processes));*/

   if (argc < 2)
   {
      fprintf(stderr, "You must pass a path to execute the command.\n");
      return  -1;
   }
   eio_file_ls(argv[1], _filter_cb, _main_cb, _done_cb, _error_cb,
         &number_of_listed_files);
   printf("%s\n", "hello");


   ecore_main_loop_begin();
   eina_list_free(processes);
   eio_shutdown();
   ecore_shutdown();

   return 0;
}

process_t
get_process_information(const char * file)
{
   process_t p = { 0, NULL, 0, 0};
   char * stat_path = NULL;
   FILE *f = NULL;
   char buf[200];
   int i;

   // Get pid
   char *last_slash = strrchr(file, '/');
   /*printf("Processing : %s\n", ++last_slash);*/
   p.pid = atoi(++last_slash);

   // Get name
   stat_path = (char *)malloc(sizeof(char)*(strlen(file) + strlen("/stat")));
   strcat(stat_path, file);
   strcat(stat_path, "/stat");

   f = fopen(stat_path, "r+");
   if (!f) return p;

   while (fgets(buf, sizeof(buf), f) != NULL)
   {
      buf[strlen(buf) - 1] = '\0';
      printf("%s\n", buf);
   }

   fclose(f);

   for (char *p = strtok(buf," "); p != NULL; p = strtok(NULL, " "))
   {
     printf("%s\n", p);
   }

   return p;
}
