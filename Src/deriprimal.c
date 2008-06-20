/* $Id$ */

/*
 * This work was produced at the University of California, Lawrence 
 * Livermore National Laboratory (UC LLNL) under contract no. 
 * W-7405-ENG-48 (Contract 48) between the U.S. Department of Energy 
 * (DOE) and The Regents of the University of California (University) 
 * for the operation of UC LLNL. Copyright is reserved to the University 
 * for purposes of controlled dissemination, commercialization through 
 * formal licensing, or other disposition under terms of Contract 48; 
 * DOE policies, regulations and orders; and U.S. statutes. The rights 
 * of the Federal Government are reserved under Contract 48 subject to 
 * the restrictions agreed upon by the DOE and University as allowed 
 * under DOE Acquisition Letter 97-1.
 * 
 * 
 * DISCLAIMER
 * 
 * This work was prepared as an account of work sponsored by an agency 
 * of the United States Government. Neither the United States Government 
 * nor the University of California nor any of their employees, makes 
 * any warranty, express or implied, or assumes any liability or 
 * responsibility for the accuracy, completeness, or usefulness of any 
 * information, apparatus, product, or process disclosed, or represents 
 * that its use would not infringe privately-owned rights.  Reference 
 * herein to any specific commercial products, process, or service by 
 * trade name, trademark, manufacturer or otherwise does not necessarily 
 * constitute or imply its endorsement, recommendation, or favoring by 
 * the United States Government or the University of California. The 
 * views and opinions of authors expressed herein do not necessarily 
 * state or reflect those of the United States Government or the 
 * University of California, and shall not be used for advertising or 
 * product endorsement purposes.
 * 
 */

/*****************************************************************
 * TAG( DerivedMenu )
 *
 * Set up the Griz's derived menu iterms.
 */
static void
DerivedMenu( Widget parent )
{
    Widget derived_menu, label;
    int i, j, n;
    Arg args[1];
    int qty_candidates;
    int rval;
    Bool_type have_derived_result;

    NString *derived;
    char    cbuf[24];
    char    *long_name, *short_name;
    int	    k, idx, position;
    Widget  result_cascade, submenu_cascade, submenu, cascade, button;
    Bool_type make_submenu;

    /*
     * Traverse the possible_results[] array to get the names of derived
     * results in a reasonable order.  Ignore the ones that don't exist
     * since this database doesn't support them.
     */
    
    n = 0;
    have_derived_result = FALSE;
    derived_menu_widg = XmCreatePulldownMenu( parent, "derived_menu_pane", 
                                              args, n );

    derived = (NString *) malloc( 50*(sizeof(struct _nstring)) );
    memset( derived, 0, sizeof(struct _nstring) * 50 );

    j = get_derived_result_name( derived );

    for( k = 0; *derived[k].class != NULL; k++ )
    { 
       strcpy( cbuf, derived[k].class );

       /* See if submenu exists. */
       make_submenu = !find_labelled_child( derived_menu_widg, cbuf, 
                                         &submenu_cascade, &position );

       if ( make_submenu )
       {
           n = 0;
           submenu = XmCreatePulldownMenu( derived_menu_widg, "submenu_pane", 
                                           args, n );
    
           n = 0;
           XtSetArg( args[n], XmNsubMenuId, submenu ); n++;
           cascade = XmCreateCascadeButton( derived_menu_widg, cbuf, args, n );
           XtManageChild( cascade );
       }
       else
       {
           /* Submenu exists; get the pane from the cascade button. */
           XtVaGetValues( submenu_cascade, XmNsubMenuId, &submenu, NULL );
       }

       /* Add derived result button. */

       n = 0;
       button = XmCreatePushButtonGadget( submenu, 
                                          derived[k].firstlname,
                                          args, n );
       XtManageChild( button );
       XtAddCallback( button, XmNactivateCallback, res_menu_CB, 
                      derived[k].firstsname );

       have_derived_result = TRUE;

    }

    /* If no derived results available, create a noop entry for menu. */
    if ( !have_derived_result )
    {
        n = 0;
        label = XmCreateLabelGadget( derived_menu_widg, "(none available)", 
                                     args, n );
        XtManageChild( label );
    }
}


/*****************************************************************
 * TAG( PrimalMenu )
 *
 * Set up the Griz's primal menu iterms.
 */
static void
PrimalMenu( Widget parent )
{
    int i, j, k, n;
    Arg args[1];
    int qty_candidates;
    int rval;
    Widget primal_menu;
    int srec_qty, subrec_qty;
    char **svar_names;
    int dbid;

    char	cbuf[24];
    char	*long_name, short_name[24];
    char	**p_specs;
    int		idx, position;
    int		spec_qty = 0;

    Analysis 	*analy;
    NString	*primal;
    Widget	primal_cascade, submenu_cascade, submenu, result_menu;
    Widget	cascade, button;
    Bool_type	make_submenu;

    Subrecord subrec;

    n = 0;
    primal_menu_widg = XmCreatePulldownMenu( parent, "primal_menu_pane", 
                                             args, n );
  
    /*
     * Traverse the state record formats and read each subrecord definition
     * to get (potential) Primal_results in a reasonable order.
     */

    /* qty_cell_nums = sizeof( cell_nums ) / sizeof( cell_nums[0] ); */

    primal = (NString *) malloc( 50*sizeof(struct _nstring) );
    memset( primal, 0, sizeof(struct _nstring) * 50 );

    analy = env.curr_analy;

    idx = get_primal_result_name( primal );
    
    p_specs = analy->component_menu_specs;
    spec_qty = analy->component_spec_qty;

    for ( k = 0; *primal[k].class != NULL; k++ )
    {
	strcpy( cbuf, primal[k].class );
	fprintf(stderr, "the current k = %d \n", k);

    	/* See if submenu exists. */
    	make_submenu = !find_labelled_child( primal_menu_widg, cbuf, 
                                         &submenu_cascade, &position );

    	if ( make_submenu )
    	{
           n = 0;
           submenu = XmCreatePulldownMenu( primal_menu_widg, "submenu_pane", args,
                                           n );
    
           n = 0;
           XtSetArg( args[n], XmNsubMenuId, submenu ); n++;
           submenu_cascade = XmCreateCascadeButton( primal_menu_widg, cbuf, args, 
                                                    n );
           XtManageChild( submenu_cascade );
	}
	else
	{
           /* Submenu exists; get the pane from the cascade button. */
           XtVaGetValues( submenu_cascade, XmNsubMenuId, &submenu, NULL );
	}
    
	/* Now add the new primal result button. */
	/*
	if ( p_pr->var->agg_type == VECTOR
       	     || p_pr->var->agg_type == ARRAY 
       	        && p_pr->var->rank == 1 
       	        && p_pr->var->dims[0] <= qty_cell_nums )
	*/
	if ( *primal[k].secondlname != NULL ) 
	{
	   strcpy(short_name, primal[k].firstsname);
           /* Non-scalar types require another submenu level. */
           n = 0;
           result_menu = XmCreatePulldownMenu( submenu, "submenu_pane", args, n );
        
	   if ( (strncmp(primal[k].secondlname, "[", 1) != 0) )
	   {
		for ( j = k; strcmp(short_name, primal[j].firstsname) == 0; j++)
		{

                   /* Build/save complete result specification string. */
		   p_specs = RENEW_N( char *, p_specs, spec_qty, 1,
               	   	              "Extend menu specs" );


	   	   sprintf( cbuf, "%s[%s]", primal[j].firstsname, 
               				    primal[j].secondsname );
		   griz_str_dup( p_specs + spec_qty, cbuf );

		   /* Create button. */
		   n = 0;
		   button = XmCreatePushButtonGadget( result_menu, 
                       		                      primal[j].secondlname, args,
               		                              n );
		   XtManageChild( button );
		   XtAddCallback( button, XmNactivateCallback, res_menu_CB, 
               		            p_specs[spec_qty] );

		   fprintf(stderr, "p_specs(%s) \n", p_specs[spec_qty]);

		   spec_qty++;		
		   memset( cbuf, 0, 24 );
		}
		k = j - 1; /* Compensate the additional k++ at the end of loop */
	   }
	   else if ( (strncmp(primal[k].secondlname, "[", 1) == 0 ) )
	   {
		for ( j = k; strcmp(short_name, primal[j].firstsname) == 0; j++)
		{
		   /* Build/save complete result specification string. */
		   p_specs = RENEW_N( char *, p_specs, spec_qty, 1,
               		              "Extend menu specs" );
		   sprintf( cbuf, "%s%s", primal[k].firstsname, 
				          primal[k].secondsname );
              
		   griz_str_dup( p_specs + spec_qty, cbuf );
		   /* Create button. */
		   n = 0;
		   /*
		   button = XmCreatePushButtonGadget( result_menu, cell_nums[i], 
					           args, n );
		   */
		   button = XmCreatePushButtonGadget( result_menu, 
						      primal[k].secondlname, args,
						      n );
		   XtManageChild( button );
		   XtAddCallback( button, XmNactivateCallback, res_menu_CB, 
			          p_specs[spec_qty] );
		
		   spec_qty++;		
		   memset( cbuf, 0, 24 );
                
		}
		k = j - 1; /* Compensate the additional k++ at the end of loop */
	   }
        
           n = 0;
           XtSetArg( args[n], XmNsubMenuId, result_menu ); n++;
	   /*
             cascade = XmCreateCascadeButton( submenu, p_pr->long_name, args, n );
	    */

           cascade =XmCreateCascadeButton(submenu, primal[k].firstlname, args, n);
           XtManageChild( cascade );
        
           /* Update analy ("p_specs" could have been re-located). */

	   /*
            analy->component_menu_specs = p_specs;
            analy->component_spec_qty = spec_qty;
	    */
       }	 /* else if ( p_pr->var->agg_type == SCALAR ) */
       else if ( *primal[k].secondlname == NULL )
       {
           n = 0;
           button = XmCreatePushButtonGadget( submenu, primal[k].firstlname, 
					      args, n );
           XtManageChild( button );
           XtAddCallback( button, XmNactivateCallback, res_menu_CB, 
                          primal[k].firstsname );
       }
       else
           popup_dialog( INFO_POPUP, "Array/Vector Array variable \"%s\"\n%s", 
                         primal[k].firstlname, "not included in pulldown menu." );

    }   /* end of primal[k].class != NULL */
}

