/*********************************************************************************
* Copyright (C) 2017-2018 Alexey V. Akimov
*
* This file is distributed under the terms of the GNU General Public License
* as published by the Free Software Foundation, either version 2 of
* the License, or (at your option) any later version.
* See the file LICENSE in the root directory of this distribution
* or <http://www.gnu.org/licenses/>.
*
*********************************************************************************/
/**
  \file nHamiltonian_compute_adiabatic.cpp
  \brief The file implements the calculations of the adiabatic Hamiltonian (e.g. as a transformation)
    
*/


#include <stdlib.h>

#include "nHamiltonian.h"
#include "../../math_meigen/libmeigen.h"


/// liblibra namespace
namespace liblibra{

/// libhamiltonian namespace 
namespace libhamiltonian{

/// libhamiltonian_generic namespace 
namespace libhamiltonian_generic{

using namespace liblinalg;
using namespace libmeigen;


void nHamiltonian::compute_adiabatic(int der_lvl){

  compute_adiabatic(der_lvl, 0);

}

void nHamiltonian::compute_adiabatic(int der_lvl, int lvl){
/**
  Compute the adiabatic Hamiltonian

  |psi_adi> = |psi_dia> * U

  Stationary SE:  H |psi_adi> =  |psi_adi> * E

  project on <psi_adi|, keeping in mind that <psi_adi|psi_adi> = I

  <psi_dia| H |psi_adi> =  <psi_dia|psi_adi> E  

  <psi_dia| H |psi_dia> * U =  <psi_dia|psi_dia> * U * E  

  H_dia * U = ovlp_dia * U * H_adi

  Here, U = basis_transform

  Assume: H does not depend on U

  der_lvl >= 0 - only diabatic - to - adiabatic transform
  der_lvl >= 1 - forces and derivative couplings
  der_lvl >= 2 - Hessian

*/

  int i,j;

  if(level==lvl){
 
  if(der_lvl>=0){

    if(ham_dia_mem_status==0){ cout<<"Error in compute_adiabatic(): the diabatic Hamiltonian matrix is not allocated \
    but it is needed for the calculations\n"; exit(0); }
    if(ovlp_dia_mem_status==0){ cout<<"Error in compute_adiabatic(): the overlap matrix of the diabatic states is not allocated \
    but it is needed for the calculations\n"; exit(0); }

    if(ham_adi_mem_status==0){ cout<<"Error in compute_adiabatic(): the adiabatic Hamiltonian matrix is not allocated \
    but it is used to collect the results of the calculations\n"; exit(0); }
    if(basis_transform_mem_status==0){ cout<<"Error in compute_adiabatic(): the basis_transform (eigenvector) matrix is\
    not allocated but it is used to collect the results of the calculations\n"; exit(0); }


    if(nadi==1 && ndia==1){
      *ham_adi = *ham_dia;
      basis_transform->set(0,0, 1.0, 0.0);
    }
    else{   solve_eigen(ham_dia, ovlp_dia, ham_adi, basis_transform, 0);   }

    if(der_lvl>=1){

      // Now compute the derivative couplings (off-diagonal, multiplied by energy difference) and adiabatic gradients (diagonal)
      CMATRIX* tmp; tmp = new CMATRIX(nadi,nadi);
      CMATRIX* dtilda; dtilda = new CMATRIX(nadi,nadi);
      
      for(int n=0;n<nnucl;n++){

        if(d1ham_dia_mem_status[n]==0){ cout<<"Error in compute_adiabatic(): the derivatives of the diabatic Hamiltonian \
        matrix w.r.t. the nuclear DOF "<<n<<" is not allocated but is needed for calculations \n"; exit(0); }

        if(d1ham_adi_mem_status[n]==0){ cout<<"Error in compute_adiabatic(): the derivatives of the adiabatic Hamiltonian \
        matrix w.r.t. the nuclear DOF "<<n<<" is not allocated but is needed for collecting results \n"; exit(0); }


        if(dc1_dia_mem_status[n]==0){ cout<<"Error in compute_adiabatic(): the derivatives couplings matrix in the diabatic \
        basis w.r.t. the nuclear DOF "<<n<<" is not allocated but is needed for collecting results \n"; exit(0); }

        if(dc1_adi_mem_status[n]==0){ cout<<"Error in compute_adiabatic(): the derivatives couplings matrix in the adiabatic \
        basis w.r.t. the nuclear DOF "<<n<<" is not allocated but is needed for collecting results \n"; exit(0); }


        // E.g. see the derivations here: https://github.com/alexvakimov/Derivatory/blob/master/theory_NAC.pdf
        // also: http://www.theochem.ruhr-uni-bochum.de/~nikos.doltsinis/nic_10_doltsinis.pdf
        *tmp = (*basis_transform).H() * (*d1ham_dia[n]) * (*basis_transform);

        *dtilda = (*basis_transform) * (*dc1_dia[n]).H() * (*basis_transform).H() * (*ham_adi);
        *dtilda = (*dtilda + (*dtilda).H() );
        *tmp -= *dtilda;

        // Adiabatic "forces"
        *d1ham_adi[n] = 0.0;
        for(i=0;i<nadi;i++){ d1ham_adi[n]->set(i,i, tmp->get(i,i)); }

        // Adiabatic derivative couplings
        *dc1_adi[n] = 0.0;

        for(i=0;i<nadi;i++){
          for(j=0;j<nadi;j++){

            if(i==j){  dc1_adi[n]->set(i,j, 0.0, 0.0); }
            else{
 
              double dE = (ham_adi->get(j,j) - ham_adi->get(i,i) ).real();
              if(fabs(dE)<1e-10){ 
                //dE = 1e-10 * (dE>0.0 ? 1.0 : -1.0); 
                dc1_adi[n]->set(i,j, 0.0, 0.0 );
              }else{
          
                dc1_adi[n]->set(i,j, tmp->get(i,j)/dE );
              }

            }
          }// for j
        }// for i
      }// for n

      delete tmp;
      delete dtilda;

    }// der_lvl>=1
  }// der_lvl>=0

  }// level == lvl

  else if(lvl>level){
  
    for(int i=0;i<children.size();i++){
      children[i]->compute_adiabatic(der_lvl, lvl);
    }

  }// lvl >level

  else{
    cout<<"WARNING in nHamiltonian::compute_adiabatic\n"; 
    cout<<"Can not run evaluation of function in the parent Hamiltonian from the\
     child node\n";    
  }
     
}



void nHamiltonian::compute_adiabatic(bp::object py_funct, bp::object q, bp::object params){
/**
  Performs the adiabatic properties calculation at the top-most level of the Hamiltonians 
  hierarchy. See the description of the more general function prototype for more info.
*/ 

  compute_adiabatic(py_funct, q, params, 0);

}


void nHamiltonian::compute_adiabatic(bp::object py_funct, bp::object q, bp::object params, int lvl){
/**
  This function will call the <py_funct> function defined in Python and taking the signature:

  def py_funct(q, params, full_id)
      return obj

  The object <obj> returned by the function should contain variables named:
  "ham_adi" (CMATRIX), "d1ham_adi" (list of CMATRIX), "dc1_adi" (list of CMATRIX)
  of the specified types

  q - is the object containing coordinates of nuclei. Since almost any C++ type exposed to Python is seen 
  by Python as a Python object, you can use any convenient Libra type to store the coordinates, for instance
  a MATRIX object. 

  params - is an object containing any parameters needed by the <py_funct> Python function to perform
  the specified calculations. Again, there are many options to pass the parameters on the Python side, but 
  it may be convenient to use Python dictionary, since the parameters will be mostly used at the 
  Python level of calculations.

  full_id - is vector<int> object containing the "path" of the calling node in the hierarchy of Hamiltonians
  this information may be needed by the py_funct function in order to do the proper calculations

  lvl - is the level of the Hamiltonians in the hierarchy of Hamiltonians to be executed by this call 
  You can only request to perform computations at the same of higher (children) levels that the level of 
  the Hamiltonian from which you call this function. 

  Contentwise, this function computes the adiabatic properties and populates the corresponding
  storage.

*/

  if(level==lvl){

    // Temporary storage
    CMATRIX _Hadi(nadi,nadi);
    vector<CMATRIX> _d1ham_adi(nnucl, CMATRIX(nadi,nadi));
    vector<CMATRIX> _dc1_adi(nnucl, CMATRIX(nadi,nadi));
  
    // Call the Python function with such arguments
    bp::object obj = py_funct(q, params, get_full_id() );  
  
    // Extract all the computed properties
    int has_attr=0;
    has_attr = (int)hasattr(obj,"ham_adi");        
    if(has_attr){    _Hadi = extract<CMATRIX>(obj.attr("ham_adi"));    }
  
    has_attr=0;
    has_attr = (int)hasattr(obj,"d1ham_adi");        
    if(has_attr){    _d1ham_adi = extract<CMATRIXList>(obj.attr("d1ham_adi"));    }
  
    has_attr=0;
    has_attr = (int)hasattr(obj,"dc1_adi");        
    if(has_attr){    _dc1_adi = extract<CMATRIXList>(obj.attr("dc1_adi"));    }
  
  
    // Copy the temporary results into the Hamiltonian-bound memory
    *ham_adi = _Hadi;
    
    for(int i=0;i<nnucl;i++){
      *d1ham_adi[i] = _d1ham_adi[i];
      *dc1_adi[i]   = _dc1_adi[i];
    }
  
  }// if lvl == level

  else if(lvl>level){
  
    for(int i=0;i<children.size();i++){
      children[i]->compute_adiabatic(py_funct,q,params,lvl);
    }

  }

  else{
    cout<<"WARNING in nHamiltonian::compute_adiabatic\n"; 
    cout<<"Can not run evaluation of function in the parent Hamiltonian from the\
     child node\n";    
  }


}






}// namespace libhamiltonian_generic
}// namespace libhamiltonian
}// liblibra

