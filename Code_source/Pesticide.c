/*----------Program for pesticides flux calculations--------

References:
Lichiheb, N., Personne, E., Bedos, C., Barriuso, E., 2014. 
Adaptation of a resistive model to pesticide volatilization 
from plants at the field scale: Comparison with a dataset. 
Atmospheric Environment 83, 260�268. https://doi.org/10.1016/j.atmosenv.2013.11.004

Lichiheb, N., Personne, E., Bedos, C., Van den Berg, F., Barriuso, E., 2016. 
Implementation of the effects of physicochemical properties on the foliar penetration 
of pesticides and its potential for estimating pesticide volatilization from plants. 
Science of The Total Environment 550, 1022�1031. https://doi.org/10.1016/j.scitotenv.2016.01.058
// WARNING - Be careful at the Unit for water solubility in the equation 19 / error in the paper - equation 19 derived from expe. integrates water solubility in g/L and not in mg/L
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "variables_parametres.h"
#include "fonctions.h"

void Pest_resistance_calculations(void);
void Pest_stock_application_calculations(void);
void Pest_flux_calculations(void);
void Pest_stock_evolution_calculations(void);


void Flux_Pesticide()
{

	Pest_resistance_calculations();

	sprintf(&Date_Time_Pest_application, "%s %s", Pest_Application_Date, Pest_Application_Time);
	
	if (strcmp(Date_Time_Pest_application, Date_Time) == 0)
		Pest_stock_application_calculations();

	Pest_flux_calculations();
	Pest_stock_evolution_calculations();

}

void Pest_resistance_calculations()
{

	// Stomatal resistances are calculated, but no stomatal flux is currently estimated. For eventual futur use
	Rstom_Pest = diffusivity_H2O / diffusivity_Pest * Rstom_H2O * K_stom_absorption_Pest;
	Rstom_inactive_Pest = diffusivity_H2O / diffusivity_Pest * Rstom_inactive_H2O * K_stom_absorption_Pest;

	Rb_leaf_Pest = diffusivity_H2O / diffusivity_Pest * Rb_leaf;

	Rb_soil_Pest = 2.0 / (karman * u_star_ground) * pow((Sc_Pest / Prandt), 2 / 3);
	if (Rb_soil_Pest > 3000.0)
		Rb_soil_Pest = 3000.0;

	// calculation of the saturated pressure at the temperature of the leaf 
	ex_Pest = ex_Pest_Tref * exp(Vaporization_enthalpy_Pest * (1.0 / (Tref_Pest + T0C) - 1.0 / (T_leaf + T0C)) / R);

	// Calculation of the concentration of the Pesticid at the leaf surface (1E6 = conversion g/m3 => �g/m3)
	Pest_leaf = Molar_mass_Pest * ex_Pest / R / (T_leaf + T0C) * 1.0E6;

}

void Pest_stock_application_calculations()
{

	// distribution of the Pesticid solution over the leaf surface and soil surface

	// WARNING : for the moment total interception by the foliage (nothing arrives on the soil)
	//Pest_quantity_leaf = Pest_quantity_applied - Pest_quantity_applied * exp(-0.4*LAI_total); <= test (without mechanistic approaochfor  
	Pest_quantity_leaf = Pest_quantity_applied;
	Pest_quantity_soil = Pest_quantity_applied - Pest_quantity_leaf;


	// distribution of the pesticid between the various reservoir in the plant/tissue - adsorption of Pest on the cuticle
	if (logKow_Pest >= 2.43 && logKow_Pest <= 3.85)
		Pest_quantity_leaf_adsorb = ((0.7* logKow_Pest) - 1.7) * (Pest_quantity_leaf);
	else if (logKow_Pest < 2.43)
		Pest_quantity_leaf_adsorb = 0.0;
	else if (logKow_Pest > 3.85)
		Pest_quantity_leaf_adsorb = Pest_quantity_leaf;

	Pest_quantity_leaf_non_adsorb = max(0.0, Pest_quantity_leaf - Pest_quantity_leaf_adsorb);
}

void Pest_flux_calculations()
{
	Pest_canopy = (((1.0/Rcut_Pest) + (1.0 / Rb_leaf_Pest)) * (concentration_Pest / Ra + Pest_soil / (Rac + Rb_soil_Pest)) + ((1.0/Rcut_Pest) * Pest_leaf * Pest_quantity_leaf_non_adsorb / Rb_leaf_Pest) / Lambda_Pest) / (((1.0/Rcut_Pest) + (1.0 / Rb_leaf_Pest)) * (1.0 / Ra + 1.0 / (Rac + Rb_soil_Pest) + 1.0 / Rb_leaf_Pest) - ((1.0 / Rb_leaf_Pest)*(1.0 / Rb_leaf_Pest)));

	FPest_soil = -(Pest_canopy - Pest_soil) / (Rac + Rb_soil_Pest);
	FPest_canopy = -(Pest_canopy - (Pest_leaf*Pest_quantity_leaf_non_adsorb / Lambda_Pest)) / Rb_leaf_Pest;
	FPest_tot = -(concentration_Pest - Pest_canopy) / Ra;

	FPest_washoff = Pest_quantity_leaf_non_adsorb * K_washoff_Pest * (Rain / delta_time * 3600.0);

	K_photodegrad_Pest = (Rg / 500.0) * K_photodegrad_Pest_ref;
	if (Rg < 0.001 && K_photodegrad_Pest_ref < 0.001)
		K_photodegrad_Pest = 0.000000000001;
	FPest_leaf_photodegrad_adsorb = Pest_quantity_leaf_adsorb * K_photodegrad_Pest;
	FPest_leaf_photodegrad_non_adsorb = Pest_quantity_leaf_non_adsorb * K_photodegrad_Pest;

	K_penetration_Pest = (0.3 * Solub_Water_Pest + 0.17) / (24.0*3600.0);
	if (Pest_quantity_leaf > 0.0)
	{
		FPest_leaf_penetration_adsorb = Pest_quantity_leaf_adsorb * K_penetration_Pest;
		FPest_leaf_penetration_non_adsorb = Pest_quantity_leaf_non_adsorb * K_penetration_Pest;
	}

	FPest_leaf_dissip = K_leaf_dissip_Pest * Pest_quantity_leaf_Tissue;

	// Controle of the values of the fluxes/concentrations for very low canopies
	if ((zh - z0_soil) <= 0.03)
	{
		Pest_canopy = 0.000001;
		Pest_leaf = 0.000001;
		FPest_soil = FPest_tot;
		FPest_canopy = -0.000001;
		FPest_washoff = 0.000001;
		FPest_leaf_photodegrad_adsorb = 0.000001;
		FPest_leaf_photodegrad_non_adsorb = 0.000001;
		FPest_leaf_penetration_adsorb = 0.000001;
		FPest_leaf_penetration_non_adsorb = 0.000001;
		FPest_leaf_dissip = 0.000001;	
	}



}

void Pest_stock_evolution_calculations()
{
	// distribution of the pesticid between the various reservoir in the plant/tissue - adsorption of Pest on the cuticle
	Pest_quantity_leaf = Pest_quantity_leaf_adsorb + Pest_quantity_leaf_non_adsorb;

	if (logKow_Pest >= 2.43 && logKow_Pest <= 3.85)
		Pest_quantity_leaf_adsorb = ((0.7* logKow_Pest) - 1.7) * (Pest_quantity_leaf);
	else if (logKow_Pest < 2.43)
		Pest_quantity_leaf_adsorb = 0.0;
	else if (logKow_Pest > 3.85)
		Pest_quantity_leaf_adsorb = Pest_quantity_leaf;

	Pest_quantity_leaf_non_adsorb = max(0.0, Pest_quantity_leaf - Pest_quantity_leaf_adsorb);

	Pest_quantity_leaf_non_adsorb = max(0.0, Pest_quantity_leaf_non_adsorb - (FPest_canopy + FPest_washoff + FPest_leaf_photodegrad_non_adsorb + FPest_leaf_penetration_non_adsorb) * delta_time * 3600.0);

	Pest_quantity_leaf_adsorb = max(0.0, Pest_quantity_leaf_adsorb - (FPest_leaf_photodegrad_adsorb + FPest_leaf_penetration_adsorb) * delta_time * 3600.0);

	Pest_quantity_leaf_Tissue = max(0.0, Pest_quantity_leaf_Tissue + (FPest_leaf_penetration_adsorb + FPest_leaf_penetration_non_adsorb - FPest_leaf_dissip) * delta_time * 3600.0);

	// Controle of the values of the fluxes/concentrations for very low canopies
	if ((zh - z0_soil) <= 0.03)
	{
		Pest_quantity_leaf = 0.000001;
		Pest_quantity_leaf_adsorb = 0.000001;
		Pest_quantity_leaf_non_adsorb = 0.000001;
		Pest_quantity_leaf_Tissue = 0.000001;
	}
}