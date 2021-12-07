/*
 *
 Copyright (c) 2016, Lawrence Livermore National Security, LLC. 
 Produced at the Lawrence Livermore National Laboratory. Written 
 by Kevin Durrenberger: durrenberger1@llnl.gov. CODE-OCEC-16-056. 
 All rights reserved.

 This file is part of Mili. For details, see <URL describing code 
 and how to download source>.

 Please also read this link-- Our Notice and GNU Lesser General 
 Public License.

 This program is free software; you can redistribute it and/or modify 
 it under the terms of the GNU General Public License (as published by 
 the Free Software Foundation) version 2.1 dated February 1999.

 This program is distributed in the hope that it will be useful, but 
 WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF 
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms 
 and conditions of the GNU General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License 
 along with this program; if not, write to the Free Software Foundation, 
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 
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

#ifndef TAURUS_DB_H
#define TAURUS_DB_H


#include "mili.h"

/*****************************************************************
 * TAG( K_EPSILON_MASK A2_MASK )
 *
 * Bit-flag masks for additional state data.
 */
#define K_EPSILON_MASK 0x1
#define A2_MASK 0x2
void fix_title( char * );
static char *global_primal_shorts[] =
{
   "ke", "pe", "te", "rigx", "rigy", "rigz", NULL
};
static char *global_primal_longs[] =
{
   "Kinetic Energy",
   "Potential Energy",
   "Total Energy",
   "Rigid-body X Velocity",
   "Rigid-body Y Velocity",
   "Rigid-body Z Velocity",
   NULL
};
static char *mat_primal_shorts[] =
{
   "matpe", "matke", "matmass", "matv", "matxv", "matyv", "matzv", NULL
};
static char *mat_primal_longs[] =
{
   "Material Potential Energy",
   "Material Kinetic Energy",
   "Material Mass",
   "Material Velocity",
   "Material Rigid-body X Velocity",
   "Material Rigid-body Y Velocity",
   "Material Rigid-body Z Velocity",
   NULL
};
static char *mat_primal_subrec_shorts[] =
{
   "matpe", "matke", "matv", "matmass", NULL
};
static char *wall_primal_shorts[] =
{
   "wf", NULL
};
static char *wall_primal_longs[] =
{
   "Rigid Wall Force", NULL
};
static char *nike_primal_shorts[] =
{
   "slide", NULL
};
static char *nike_primal_longs[] =
{
   "Slide Surface Force", NULL
};
static char *node_primal_shorts[] =
{
   "nodpos", "nodvel", "nodacc", NULL
};
static char *node_primal_longs[] =
{
   "Node Position", "Node Velocity", "Node Acceleration", NULL
};
static char *node_posit_shorts[] =
{
   "ux", "uy", "uz", NULL
};
static char *node_posit_longs[] =
{
   "X Position", "Y Position", "Z Position", NULL
};
static char *node_vel_shorts[] =
{
   "vx", "vy", "vz", NULL
};
static char *node_vel_longs[] =
{
   "X Velocity", "Y Velocity", "Z Velocity", NULL
};
static char *node_acc_shorts[] =
{
   "ax", "ay", "az", NULL
};
static char *node_acc_longs[] =
{
   "X Acceleration", "Y Acceleration", "Z Acceleration", NULL
};
static char *temp_short[] =
{
   "temp", NULL
};
static char *temp_long[] =
{
   "Temperature", NULL
};
static char *flow_short[] =
{
   "k", "eps", "a2", NULL
};
static char *flow_long[] =
{
   "K", "Epsilon", "A2", NULL
};
static char *hex_primal_shorts[] =
{
   "stress", "eeff",  NULL
};
static char *hex_primal_longs[]=
{
   "Stress",
   "Eff plastic strain",
   NULL
};
static char *hex_stress_shorts[] =
{
   "sx", "sy", "sz", "sxy", "syz", "szx", NULL
};
static char *hex_stress_longs[] =
{
   "Sigma-xx", "Sigma-yy", "Sigma-zz",
   "Sigma-xy", "Sigma-yz", "Sigma-zx",
   NULL
};
static char *beam_primal_shorts[]=
{
   "axfor", "sshear", "tshear", "smom", "tmom", "tor", NULL
};
static char *beam_primal_longs[]=
{
   "Axial Force", "Shear Resultant - s",
   "Shear Resultant - t", "Bending Moment  - s",
   "Bending Moment  - t", "Torsional Resultant", NULL
};
static char *shell_primal_shorts[] =
{
   "stress_mid",
   "eeff_mid",
   "stress_in",
   "eeff_in",
   "stress_out",
   "eeff_out",
   "bend",
   "shear",
   "normal",
   "thick",
   "edv1",
   "edv2",
   "inteng",
   "strain_in",
   "strain_out",
   "unused_var",
   NULL
};
static char *shell_primal_longs[]=
{
   "Middle Stress", "Middle Eff Plastic Strain",
   "Inner Stress", "Inner Eff Plastic Strain",
   "Outer Stress", "Outer Eff Plastic Strain",
   "Bending Resultant", "Shear Resultant",
   "Normal Resultant",
   "Thickness",
   "Element variable 1", "Element variable 2",
   "Internal Energy",
   "Inner Strain", "Outer Strain",
   "Unused var",
   NULL
};
static char *stress_comp_shorts[] =
{
   "sx", "sy", "sz", "sxy", "syz", "szx", NULL
};
static char *stress_comp_longs[] =
{
   "Sigma-xx", "Sigma-yy", "Sigma-zz",
   "Sigma-xy", "Sigma-yz", "Sigma-zx",
   NULL
};
static char *bend_comp_shorts[] =
{
   "mxx", "myy", "mxy", NULL
};
static char *bend_comp_longs[] =
{
   "Mxx", "Myy", "Mxy", NULL
};
static char *shear_comp_shorts[] =
{
   "qxx", "qyy", NULL
};
static char *shear_comp_longs[] =
{
   "Qxx", "Qyy", NULL
};
static char *normal_comp_shorts[] =
{
   "nxx", "nyy", "nxy", NULL
};
static char *normal_comp_longs[] =
{
   "Nxx", "Nyy", "Nxy", NULL
};
static char *strain_comp_short[]=
{
   "ex", "ey", "ez", "exy", "eyz", "ezx", NULL
};
static char *strain_comp_long[]=
{
   "Eps-xx", "Eps-yy", "Eps-zz",
   "Eps-xy", "Eps-yz", "Eps-zx", NULL
};
static char *sand_db_short[]=
{
   "sand", NULL
};
static char *sand_db_long[]=
{
   "Brick Sand", "Beam Sand", "Shell Sand",
   "Thick Shell Sand", "Discrete Sand", NULL
};
static int types[]=
{
   M_FLOAT, M_FLOAT, M_FLOAT,
   M_FLOAT, M_FLOAT, M_FLOAT
};

#endif /* TAURUS_DB_H */
