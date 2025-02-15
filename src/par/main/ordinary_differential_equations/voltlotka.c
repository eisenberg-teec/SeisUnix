/* Copyright (c) Colorado School of Mines, 2011.*/
/* All rights reserved.		       */

/* VOLTLOTKA: $Revision: 1.3 $ ; $Date: 2020/05/15 20:06:40 $        */

#include "par.h"
#include "rke.h"

/*********************** self documentation **********************/

char *sdoc[] = {
"								",
" VOLTLOTKA -  VOLTerra-LOTKA predator-prey model, modified 	",
"	        slightly to include a carrying capacity of the  ",
"		prey species						",
"								",
"  voltlotka  > [stdout]					",
"								",
" Required Parameters: none					",
" Optional Parameters:						",
" a=0.5			prey increase parameter			",
" b=0.02		prey reduction by predation parameter	",
" c=0.75		predator reduction rate			",
" d=0.005		predator increase by prey capture	",
"								",
" K=1000000		carrying capacity of prey environment 	",
"			reduces to classical Volterra-Lotka when",
"			K >> x and K >> y	",
"								",
" x0=500		initial number of prey			",
" y0=25			initial number of predator		",
"								",
" thresh=1.0		extinction if x <= thresh or y <= thresh",
"								",
" h=1.0			step size				",
" stepmax=100		maximum number of steps to compute	",
" mode=PP		number of Pred. followed by number of Prey",
"			=R predator only, =Y prey only		",
" Notes:							",
" This program is really just a demo showing how to use the 	",
" differential equation solver rke_solve written by Francois 	",
" Pinard, based on a modified form of the 4th order Runge-Kutta ",
" method, which employs the error checking method of R. England ",
" 1969.								",
"								",
" The output consists of unformated C-style binary floats, of	",
" either pairs or triplets as specified by the \"mode\" paramerter.",
"								",
" Modified Volterra-Lotka equations: 				",
" dx/dt = a*x*(1 - x/K) - b*x*y					",
" dy/dt = -c*y + p*x*y						",
"								",
" x = number of members of the prey species			",
" y = number of members of the predator species 		",
" bxy = prey population reduction due to predation		",
" pxy = predator population increase due due to predation	",
" a = prey increase parameter (prey have unlimited food)	",
" c = predator reduction (predators starve without prey)	",
" K = carrying capacity of for the prey specie, when K large	",
"     (1 - x/K)-> 1 and the system reduces to original VL system",
"								",
" Examples:							",
" Default: classic Snowshoe hare versus Canadian lynx		",
" voltlotka | xgraph n=100 nplot=2 d1=1 style=normal &		",
" 								",
" Caveat: if there is weird behavior, try reducing the h= value	",
NULL};

/*
 * References:
 *
 * Author:  May 2020: John Stockwell
 */

/**************** end self doc ********************************/

/* Prototype of function used internally */
static int
modified_volterra_lotka_equations(double t, double y[2] , double yprime[2]);

/* Define values of imode */
#define PP_MODE 0
#define R_MODE 1
#define Y_MODE 2

int
main(int argc, char **argv)
{
	register int i=0, j=0;		/* counters */
	register int number=2; 		/* the three dependent variables */
	int verbose=0;		/* verbose flag =1 chatty, =0 silent */
	int stepmax=0;		/* maximum number of steps */

	/* initial values of x and y */
	double x0=0.0;		/* initial value of susceptible population */
	double y0=0.0;		/* initial value of infectives */

	double t=0.0;		/* time */
	double h=.001;		/* time increment */
	double tol=0.0;		/* time increment */

	double y[2]={0.0,0.0};	/* dependent variable of ODE system */

	rke_variables p;	/* variable used by RKE routines */

	float **yout=NULL; 	/* output array */
	float *tempout=NULL;	/* temporary output array */

	FILE *out_file=stdout;	/* pointer to file that we write out to */

	cwp_String mode="PP";	/* output mode of program */
	int imode=PP_MODE;	/* integer flag for mode */


	/* Hook up getpar */
	initargs(argc, argv);
	requestdoc(0);

	switch(filestat(STDOUT)) { /* Prevent floats from dumping on screen */
	case BADFILETYPE:
		warn("stdout is illegal filetype");
		pagedoc();
	break;
	case TTY:
		warn("stdout can't be tty");
		pagedoc();
	break; 
	default:			   /* rest are OK */
	break;

	}

	/* Get parameters */
	if (!getparint("stepmax", &stepmax))	stepmax = 100;
	if (!getparint("verbose", &verbose))	verbose = 0;

	/* Initial conditions y[0] = x  y[1]= y  */
	if (!getpardouble("x0", &x0))		x0=500;
		y[0] = x0;
	if (!getpardouble("y0", &y0))		y0=25;
		y[1] = y0;

	if (!getpardouble("h", &h))		h = 1.0;
	if (!getpardouble("tol", &tol))		tol = RKE_ERR_BIAS_INIT;

	/* Get output mode, recall imode initialized to the default FABS */
	if (!getpardouble("h", &h))		h = 1.0;
	getparstring("mode", &mode);

	if (STREQ(mode, "R"))    	imode = R_MODE;
	else if (STREQ(mode, "Y"))	imode = Y_MODE;
	else if (!STREQ(mode, "PP"))
	    err("unknown operation=\"%s\", see self-doc", mode);

	/* allocate space in the output array */
	yout = ealloc2float(2,2*stepmax);

	/* zero out the array */
	memset((void *) yout[0], 0 , 2*stepmax*DSIZE);
	
	/* initialize Runge-Kutta-England routines */
	p = (rke_variables)
		rke_init(3, modified_volterra_lotka_equations);

	/* set tolerance */
	p->error_bias=tol;

	for (i=0; i<stepmax; ++i) {
		double aimed_t;
		t=i*h;
		aimed_t=t+h;
  		if (verbose) {
			warn("using %3d accepted and %3d rejected steps",
	 			p->accepted_steps, p->rejected_steps);
			if (verbose) warn("error tolerance = %10.24f",p->error_bias);
		}

		/* convert doubles in y to floats in yout and write out */
		for(j=0; j<number; ++j) yout[i][j] = (float) y[j];

		/* run the Runge-Kutta-England solver */
  		rke_solve (p, &t, y, aimed_t);
	}

	/* write out according to the mode */
	tempout = ealloc1float(2*stepmax);

	if (imode==R_MODE) {
		for (i=0; i<stepmax; ++i)
			tempout[i] = yout[i][0];
	} else if (imode==Y_MODE) {
		for (i=0; i<stepmax; ++i)
			tempout[i] = yout[i][1];
	} else if (imode==PP_MODE) {

		for (i=0; i<stepmax; ++i)
			tempout[i] = yout[i][0];
		for (i=0; i<stepmax; ++i)
			tempout[i+stepmax] = yout[i][1];
	}

	if (imode==PP_MODE) {
		efwrite(tempout,sizeof(float),2*stepmax,out_file);
	} else {
		efwrite(tempout,sizeof(float),stepmax,out_file);
	}

	/* end the session with rke */
	rke_term(p);

	return EXIT_SUCCESS;
}


static int
modified_volterra_lotka_equations(double t, double y[2] , double yprime[3])
/*********************************************************************
modified_volterra_lotka_equations - the system of ODE's describing predator-prey
**********************************************************************
t	independent variable "time"
y 	dependent variable being solved for y(t)
yprime	derivative of dependent variable  y'(t)
**********************************************************************
Notes: This is an example of an autonomous system of ODE's
**********************************************************************/
{
	double a=0.0;	/* prey increase		*/
	double b=0.0;	/* prey removal  by predation	*/
	double c=0.0;	/* predator removal rate	*/
	double p=0.0;	/* predator increase rate	*/
	double thresh=1.0;	/* threshold */	

	/* modification */
	double K=0.0;	/* prey carrying capacity	*/

	/* parameters */
	if (!getpardouble("a", &a))		a = 0.5;
	if (!getpardouble("b", &b))		b = 0.02;
	if (!getpardouble("c", &c))		c = 0.75;
	if (!getpardouble("p", &p))		p = 0.005;
	if (!getpardouble("K", &K))		K = 1000000;

	if (!getpardouble("thresh", &thresh))	thresh=1.0;

	/* Volterra-Lotka predator-prey equations */
	if (y[0] <= thresh) y[0] = 0.0;
	if (y[1] <= thresh) y[1] = 0.0;

	yprime[0] = a*y[0]*(1 - y[0]/K)  - b*y[0]*y[1];
	yprime[1] = -c*y[1]  + p*y[0]*y[1];

    return 1;
}
