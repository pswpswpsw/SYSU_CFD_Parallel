#include <iostream>
#include <fstream>
#include <math.h>
#include "navier.h"
#include "tools.h"

using namespace std;

int NavierStokesSolver::CalculatePressure( )
{
    int i,pIter=0;
    double pIterRes=0,coef=0; //, roinv,Tem;

	if( DensityModel==0 ){
//		Q_Constr(&Ap,   "matrix",    Ncel, True,  Rowws, Normal, True);
	}
	else
//		Q_Constr(&Ap,   "matrix",    Ncel, False, Rowws, Normal, True);

	// Init value for iteration
//    V_SetAllCmp(&xsol, 0.);
    // Build matrix
    BuildPressureMatrix( );
    // Solve pressure Poisson equation. Symmetric sparse linear equations solver
	if( DensityModel==0 );
//		SolveLinearEqu( CGIter,    &Ap, &xsol, &bp, 1000, SSORPrecond, 1.3, 1.e-9, &pIter, &pIterRes );
	else
//		SolveLinearEqu( GMRESIter, &Ap, &xsol, &bp, 1000, SSORPrecond, 1.3, 1.e-9, &pIter, &pIterRes );
	if( pIterRes>1. ){
		cout<<"Pressure iteration cannot converge! "<<pIter<<" "<<pIterRes<<endl;
		exit(0);
	}


	
    // update other variables
//	SetBCDeltaP( BPre, &xsol.Cmp[1] );
//     Gradient   ( &xsol.Cmp[1], BPre, dPdX );  // note the first parameter
    for( i=0; i<Ncel; i++ )
    {
//		Residual[3] += fabs( xsol.Cmp[i+1] - xsol.Cmp[cellPressureRef+1] )*Cell[i].vol;

		// pressure (and density) correction
		if(      DensityModel==0 );
//			Pn[i] = Pn[i] + URF[3] * (xsol.Cmp[i+1]-xsol.Cmp[cellPressureRef+1]);  //under-relaxation factor: 
		else if( DensityModel==1 ){ // perfect gas
//			Pn[i] = Pn[i] + URF[3] *  xsol.Cmp[i+1];
//			Rn[i] += xsol.Cmp[i+1]/(Rcpcv*Tn[i]);
			// Rn[i] = (Pn[i]+PressureReference)/(Rcpcv*Tn[i]);
		}

		// velocity correction
		coef  = Cell[i].vol*Apr[i];
        Un[i] = Un[i] - coef*dPdX[i][0];
        Vn[i] = Vn[i] - coef*dPdX[i][1];
        Wn[i] = Wn[i] - coef*dPdX[i][2];
	}

	// correct face normal velocity to satify the mass balance equ
//	CorrectRUFace2( &xsol.Cmp[1] );
	// SetBCVelocity( BRo,BU,BV,BW );
	// CalRUFace2( );
	
	// clipping work in case of divergence

//	Q_Destr ( &Ap );
    return 0;
}

void NavierStokesSolver::BuildPressureMatrix( ) //no second pressure correctio is used in this funtion .CXY e.g. equation 8.62
{
    int i,j, nj,in, cn[7], iface,bnd,rid;
    double Acn[7], roapf,lambda,lambda2,valcen,bpv,tmp,tmp2,vol, rof,Tf,RUnormal;

	SetBCVelocity( BRo,BU,BV,BW );
	CalRUFace2   ( ); // it doesn't need re-calculation ???
                        //calculated with u*   this is the m*

    for( i=0; i<Ncel; i++ )
    {
        valcen = 0.;
		bpv    = 0.;
        nj     = 0 ;

		// compressible gas, perfect gas
		// ?? d_ro/d_t = d_p/d_t /(RT) ?? source term and diagonal terms ,how to change?
		if( !IfSteady && DensityModel==1 )
		{
			// only Euler, or BDF 2nd can also be used ? Both, I guess
			valcen += -Cell[i].vol/( dt*Rcpcv*Tn[i] );
		}
        for( j=0; j<Cell[i].nface; j++ )
        {
            iface= Cell[i].face[j];
			// right hand side
			if( i==Face[iface].cell1 )
				bpv += RUFace[iface];
			else
				bpv -= RUFace[iface];

            in   = Cell[i].cell[j]; //j-face neighbor cell
            if( in<0  ){  // ???????????????????
				if( DensityModel==0 )continue;
				// DensityModel==1
				bnd= Face[iface].bnd;
				rid= Bnd[bnd].rid;
				RUnormal = RUFace[iface];
				if(      rid==2 ) // inlet
				{
					rof = BRo[bnd];
					Tf  = Tn[i];
					// valcen  +=  CYCASMIN(RUnormal,0.) / (rof*Rcpcv*Tf);
				}
				else if( rid==3 ) // outlet
				{
					rof = Rn[i];
					Tf  = Tn[i];
					valcen  +=  CYCASMAX(RUnormal,0.) / (rof*Rcpcv*Tf);
				}
			}
			else
			{
				if( i == Face[iface].cell1 ){
					lambda  = Face[iface].lambda;
					RUnormal=  RUFace[iface];
				}
				else{
					lambda  = 1.-Face[iface].lambda ;
					RUnormal= -RUFace[iface];
				}
				lambda2= 1.-lambda;
				roapf  = Rn[i]*Apr[i]*lambda + Rn[in]*Apr[in]*lambda2;
				vol    = Cell[i].vol*lambda + Cell[in].vol*lambda2;
				//-- centeral cell
				tmp    = roapf*vol*Face[iface].rlencos;
				valcen +=  -tmp;

				// incompressible flow
				//-- neighbor cell. Since it's symmetric, only consider upper triangle
				if( DensityModel==0 ){
				if( in>i ){
					nj ++ ;
					Acn[nj] = tmp;
					cn [nj] = in;
				}
				}
				// compressible correction
				// perfect gas, density change --> pressure change
				if( DensityModel==1 ){
					rof = lambda*Rn[i] + lambda2*Rn[in];
					if( RUnormal>0. ){
						Tf  = Tn [i];
						tmp2     =  -RUnormal/(rof*Rcpcv*Tf); //CXY: addition to diagnal A due to compressible correction
						valcen  += tmp2;
					}

					nj ++ ;
					Acn[nj] = tmp ;
					if( RUnormal<0. ){
						Tf  = Tn [in];
						Acn[nj] += -RUnormal/(rof*Rcpcv*Tf); //CXY: addition to neighbooring Coef due to compressible correction
					}
					cn [nj] = in;
				}
			}
        }

        // use laspack library to prepare the sparse matrix
		// (i+1)th row, nj+1 non-zero values
//        Q_SetLen  ( &Ap, i+1, nj+1 );
		// diagonal
//		Q_SetEntry( &Ap, i+1, 0, i+1, valcen );
		// off-diagonal
		for( j=1; j<=nj; j++ ){
//		Q_SetEntry( &Ap, i+1, j, cn[j]+1, Acn[j] );
		}
		// right hand side
//        bp.Cmp[i+1]= bpv;
    }
}

void NavierStokesSolver::CorrectRUFace2( double *dp )
{
	int i, c1,c2;
	double dx1[3],dx2[3],lambda,lambda2,vol,roapf,coef,dp1,dp2,rop,rof;

	// only for inner faces
	for( i=0; i<Nfac; i++ )
	{
		c1= Face[i].cell1;
		c2= Face[i].cell2;
		if( c2<0 )continue;

		lambda = Face[i].lambda;
		lambda2= 1.-lambda;
		vec_minus( dx1, Face[i].Xpac, Cell[c1].x, 3 );
		vec_minus( dx2, Face[i].Xnac, Cell[c2].x, 3 );

		// (ro+ro')*(u+u')= ro*u + ro*u' + ro'*u + ro'*u' (last term always omitted)
		// ro'*u (only compressible)
		if( DensityModel==1 ){
			rop = dp[c1]/(Rcpcv*Tn[c1])*lambda + dp[c2]/(Rcpcv*Tn[c2])*lambda2;
			rof = Rn[c1]*lambda                + Rn[c2]*lambda2;
			RUFace[i] += RUFace[i]*rop/rof;
		}
		// ro*u' (incoompressible or compressible)
		roapf = Rn[c1]*Apr[c1]*lambda + Rn[c2]*Apr[c2]*lambda2;
		vol   = Cell[c1].vol  *lambda + Cell[c2].vol  *lambda2;
		coef  = roapf*vol*Face[i].rlencos;
		dp1   = dp[c1] + URF[3]* vec_dot(dPdX[c1],dx1,3);  // must note this URF[3]
		dp2   = dp[c2] + URF[3]* vec_dot(dPdX[c2],dx2,3);
		RUFace[i] += -coef* ( dp2 - dp1 );
	}
	// check if sum of RUFace[i] in one cell is ZERO
}