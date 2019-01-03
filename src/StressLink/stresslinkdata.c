#include "pasitP.h"

#include <stdlib.h>


/**************************************************************************
 * @Function: stresslink_data_new( int num_node_sets, int num_element_sets )
 * @author Bob Corey
 *
 * @purpose: Allocate a stresslink data structure.
 **************************************************************************/
stresslink_data* stresslink_data_new( int num_node_sets, int num_element_sets )
{
   stresslink_data *sl = (stresslink_data*)malloc(sizeof(stresslink_data));
   sl_node_set    *node_set;
   sl_element_set *element_set;

   int i, j;

   sl->control    = NULL;
   sl->state_data = NULL;

   /* Initialize the Node Set fields */

   sl->num_node_sets  = num_node_sets;
   sl->node_sets      = (sl_node_set *) malloc(num_node_sets*sizeof(sl_node_set));

   for (i=0;
         i< num_node_sets;
         i++)
   {
      node_set             = &sl->node_sets[i];

      node_set->labels     = NULL;
      node_set->init_coord = NULL;
      node_set->cur_coord  = NULL;
      node_set->t_disp     = NULL;
      node_set->r_disp     = NULL;
      node_set->t_vel      = NULL;
      node_set->r_vel      = NULL;
      node_set->t_acc      = NULL;
      node_set->r_acc      = NULL;
   }

   /* Initialize the Element Set fields */

   sl->num_element_sets = num_element_sets;
   sl->element_sets     = (sl_element_set *) malloc(num_element_sets*sizeof(sl_element_set));

   for (i=0;
         i< num_node_sets;
         i++)
   {
      element_set              = &sl->element_sets[i];
      element_set->len_control = 0;
      element_set->control     = NULL;
      element_set->labels      = NULL;
      element_set->conn        = NULL;
      element_set->vol_ref     = NULL;
      element_set->vol_ref_over_v = NULL;
      element_set->hist_var    = NULL;
      element_set->pressure    = NULL;
      element_set->sxx         = NULL;
      element_set->syy         = NULL;
      element_set->szz         = NULL;
      element_set->sxy         = NULL;
      element_set->syz         = NULL;
      element_set->szx         = NULL;
      element_set->sx          = NULL;
      element_set->sy          = NULL;
      element_set->v           = NULL;

      /* Mat Model 11 variables */
      element_set->mat11_eps   = NULL;
      element_set->mat11_E     = NULL;
      element_set->mat11_Q     = NULL;
      element_set->mat11_vol   = NULL;
      element_set->mat11_Pev   = NULL;
      element_set->mat11_eos   = NULL;

      element_set->rhon        = NULL;
   }

   sl->num_slide_sets = 0;
   sl->slide_sets     = NULL;

   return sl;
}


/**************************************************************************
 * @Function: stresslink_data_destroy( stresslink_data* sl )
 * @author Bob Corey
 *
 * @purpose: Free up allocated space for a stresslink data structure.
 **************************************************************************/
void stresslink_data_destroy( stresslink_data* sl )
{
   int i, j;

   if ( sl->control )
      free ( sl->control );

   /* Free Node Set data */
   for ( i=0;
         i<sl->num_node_sets;
         i++ )
   {
      if ( sl->node_sets[i].len_control>0 && sl->node_sets[i].control )
         free ( sl->node_sets[i].control );

      if ( sl->node_sets[i].labels )
         free( sl->node_sets[i].labels );
      if ( sl->node_sets[i].init_coord )
         free( sl->node_sets[i].init_coord );
      if ( sl->node_sets[i].cur_coord )
         free( sl->node_sets[i].cur_coord );
      if ( sl->node_sets[i].t_disp )
         free( sl->node_sets[i].t_disp );
      if ( sl->node_sets[i].r_disp )
         free( sl->node_sets[i].r_disp );
      if ( sl->node_sets[i].t_vel )
         free( sl->node_sets[i].t_vel );
      if ( sl->node_sets[i].r_vel )
         free( sl->node_sets[i].r_vel );
      if ( sl->node_sets[i].t_acc )
         free( sl->node_sets[i].t_acc );
   }

   /* Free Element Set data */
   for ( i=0;
         i<sl->num_element_sets;
         i++ )
   {
      if ( sl->element_sets[i].len_control>0 && sl->element_sets[i].control );
      free( sl->element_sets[i].control );

      if ( sl->element_sets[i].labels )
         free( sl->element_sets[i].labels );

      if ( sl->element_sets[i].conn )
         free( sl->element_sets[i].conn );

      if ( sl->element_sets[i].vol_ref )
         free ( sl->element_sets[i].vol_ref );

      if ( sl->element_sets[i].vol_ref_over_v )
         free ( sl->element_sets[i].vol_ref_over_v );

      if ( sl->element_sets[i].hist_var )
         free( sl->element_sets[i].hist_var );

      if ( sl->element_sets[i].pressure )
         free( sl->element_sets[i].pressure );

      if ( sl->element_sets[i].sxx )
         free( sl->element_sets[i].sxx );

      if ( sl->element_sets[i].syy )
         free( sl->element_sets[i].syy );

      if ( sl->element_sets[i].szz )
         free( sl->element_sets[i].szz );

      if ( sl->element_sets[i].sx )
         free( sl->element_sets[i].sx );

      if ( sl->element_sets[i].sy )
         free( sl->element_sets[i].sy );

      if ( sl->element_sets[i].sxy )
         free( sl->element_sets[i].sxy );

      if ( sl->element_sets[i].syz )
         free( sl->element_sets[i].syz );

      if ( sl->element_sets[i].szx )
         free( sl->element_sets[i].szx );

      if ( sl->element_sets[i].v )
         free( sl->element_sets[i].v );

      /* Free up Mat Model 11 variables */

      if ( sl->element_sets[i].mat11_eps )
         free( sl->element_sets[i].mat11_eps );

      if ( sl->element_sets[i].mat11_E )
         free( sl->element_sets[i].mat11_E );

      if ( sl->element_sets[i].mat11_Q )
         free( sl->element_sets[i].mat11_Q );

      if ( sl->element_sets[i].mat11_vol )
         free( sl->element_sets[i].mat11_vol );

      if ( sl->element_sets[i].mat11_Pev )
         free( sl->element_sets[i].mat11_Pev );

      if ( sl->element_sets[i].mat11_eos )
         free( sl->element_sets[i].mat11_eos );

      if ( sl->element_sets[i].rhon )
         free( sl->element_sets[i].rhon );
   }

   if ( sl->num_node_sets>0 )
      free( sl->node_sets );
   if ( sl->num_element_sets>0 )
      free( sl->element_sets );
   if ( sl->num_slide_sets>0 )
      free( sl->slide_sets );
   free( sl );
}
