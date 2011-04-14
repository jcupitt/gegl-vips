#include <gegl.h> 

gint
main (gint    argc,
      gchar **argv)
{
  GeglNode *graph, *sink;
  GeglBuffer *buffer = NULL;
  GeglBuffer *tmpbuf = NULL;

  gegl_init (&argc, &argv);

  if (argc != 3)
    {
      g_print ("GEGL based image conversion tool\n");
      g_print ("Usage: %s <imageA> <imageB>\n", argv[0]);
      return 1;
    }

  graph = gegl_graph (sink=gegl_node ("gegl:buffer-sink", "buffer", &buffer, NULL,
                           gegl_node ("gegl:load", "path", argv[1], NULL)));
  gegl_node_process (sink);
  g_object_unref (graph);
  if (!buffer)
    {
      g_print ("Failed to open %s\n", argv[1]);
      return 1;
    }

//#define ANGLE  50.0
#define ANGLE  30.0
#define AMOUNT1 2.2
#define AMOUNT2 0.0
#define CONTRAST 0.9

  tmpbuf = buffer;
  graph = gegl_graph (sink=gegl_node ("gegl:buffer-sink", "buffer", &buffer, NULL,
                           gegl_node ("gegl:brightness-contrast", "contrast", CONTRAST, NULL,
                           gegl_node ("gegl:add", NULL,
                             gegl_node ("gegl:buffer-source", "buffer", tmpbuf, NULL),
                             gegl_node ("gegl:subtract", NULL,
                               gegl_node ("gegl:buffer-source", "buffer", tmpbuf, NULL),
                               gegl_node ("gegl:rotate", "degrees", -ANGLE, NULL,
                               gegl_node ("gegl:gaussian-blur", "std-dev-x", AMOUNT1, "std-dev-y", AMOUNT2, NULL,
                               gegl_node ("gegl:rotate", "degrees", ANGLE, NULL,
                               gegl_node ("gegl:buffer-source", "buffer", tmpbuf, NULL
                                          )))))))));
  gegl_node_process (sink);
  g_object_unref (graph);
  g_object_unref (tmpbuf);
  g_object_unref (tmpbuf);


  graph = gegl_graph (sink=gegl_node ("gegl:save",
                      "path", argv[2], NULL,
                      gegl_node ("gegl:buffer-source", "buffer", buffer, NULL)));
  gegl_node_process (sink);

  g_object_unref (buffer); 
  g_object_unref (buffer);  /* XXX: why is two unrefs needed here? */
  g_object_unref (graph);
  gegl_exit ();
  return 0;
}
