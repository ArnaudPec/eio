// gcc -o eio_file_ls eio.c `pkg-config --cflags --libs ecore eio eina` -g
/* CPU usage cal
 * https://stackoverflow.com/questions/16726779
 /how-do-i-get-the-total-cpu-usage-of-an-application-from-proc-pid-stat*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <Eio.h>
#include <Ecore.h>

static Eina_List *processesses_list, *l;
pthread_mutex_t mutex;

typedef struct process process_t;
struct process
{
   int pid;
   const char * name;
   int mem;
   double proc;
};

static Eina_Bool
get_process_information(const char * process, process_t * proc);

void
print_process(process_t * p);

static Eina_Bool
_filter_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, const char *file)
{
   char *last_slash = strrchr(file, '/');
   return (last_slash[1] > '9') ? EINA_FALSE : EINA_TRUE;
}

int
sort_cb(const void *d1, const void *d2)
{
   const process_t *p1 = d1;
   const process_t *p2 = d2;
   if(!p1) return(1);
   if(!p2) return(-1);

   return (p1->mem < p2->mem) ? 1 : -1;
}

static void
_main_cb(void *data, Eio_File *handler EINA_UNUSED, const char *file)
{
   process_t *p = NULL;
   int *number_of_listed_files = (int *)data;

   p = malloc(sizeof *p);

   get_process_information(file, p);
   pthread_mutex_lock(&mutex);

   processesses_list = eina_list_append(processesses_list, p);
   /*processesses_list = eina_list_sorted_insert(processesses_list, sort_cb, &p);*/
   print_process(p);

   pthread_mutex_unlock(&mutex);

   free(p);
   (*number_of_listed_files)++;
}

void
print_process(process_t * p)
{
   printf("%d %s %d %f\n", p->pid, p->name, p->mem, p->proc);
}

static void
_done_cb(void *data, Eio_File *handler EINA_UNUSED)
{
   int *number_of_listed_files = (int *)data;
   process_t *w;

   EINA_LIST_FOREACH(processesses_list, l, w)
       printf("%d %s %d %f\n", w->pid, w->name, w->mem, w->proc);

   printf("%s %d\n", "Number files", *number_of_listed_files);
   printf("processesses_list count: %d\n", eina_list_count(processesses_list));

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

   ecore_init();
   eio_init();
   eina_init();

   processesses_list = NULL;
   pthread_mutex_init(&mutex, NULL);

   if (argc < 2)
   {
      fprintf(stderr, "You must pass a path to execute the command.\n");
      return  -1;
   }
   eio_file_ls(argv[1], _filter_cb, _main_cb, _done_cb, _error_cb,
           &number_of_listed_files);

   ecore_main_loop_begin();
   eina_list_free(processesses_list);
   eio_shutdown();
   ecore_shutdown();

   return 0;
}

static Eina_Bool
get_process_information(const char * file, process_t * proc)
{
   FILE *f = NULL;
   char stat_path[100] = {0}, statm_path[100] = {0};
   char buf[400], bufm[400], bufu[400];
   unsigned long ttime, utime, stime, cstime, cutime, starttime, hertz;
   double secs, uptime;
   int i = 0;

   strcat(stat_path, file);
   strcat(stat_path, "/stat");

   f = fopen(stat_path, "r+");
   if (!f) {
      return EINA_FALSE;
   }

   while (fgets(buf, sizeof(buf), f) != NULL)
      buf[strlen(buf) - 1] = '\0';

   fclose(f);

   for (char *p = strtok(buf," "); p != NULL; p = strtok(NULL, " "))
   {
      switch (i++) {
         case 0:
            proc->pid = atoi(p);
            break;
         case 1:
            proc->name = p;
            break;
         case 14:
            utime = atol(p);
            break;
         case 15:
            stime = atol(p);
            break;
         case 16:
            cutime = atol(p);
            break;
         case 17:
            cstime = atol(p);
            break;
         case 22:
            starttime = atol(p);
            break;
      }
   }

   strcat(statm_path, file);
   strcat(statm_path, "/statm");

   f = fopen(statm_path, "r+");
   if (!f) {
      return EINA_FALSE;
   }

   while (fgets(bufm, sizeof(bufm), f) != NULL)
      bufm[strlen(bufm) - 1] = '\0';

   fclose(f);

   i = 0;
   for (char *p = strtok(bufm," "); p != NULL; p = strtok(NULL, " "))
   {
      switch (i++) {
         case 0:
            proc->mem = atoi(p);
            break;
      }
      break;
   }

   // CPU usage

   // hertz
   hertz = sysconf(_SC_CLK_TCK);
   // uptime

   f = fopen("/proc/uptime", "r+");
   if (!f) {
      return EINA_FALSE;
   }

   while (fgets(bufu, sizeof(bufu), f) != NULL)
      bufu[strlen(bufu) - 1] = '\0';

   fclose(f);

   i = 0;
   for (char *p = strtok(bufu," "); p != NULL; p = strtok(NULL, " "))
   {
      switch (i++) {
         case 1:
            {
               uptime = atof(p);
               break;
            }
      }
   }

   ttime = utime + stime + cstime + cutime;
   secs = (double) uptime - (double) ( (double) starttime/ (double) hertz);
   proc->proc = 100 * ( (double) ttime / (double) hertz / secs );

   return EINA_TRUE;
}
