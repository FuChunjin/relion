#ifndef CUDA_ML_OPTIMISER_H_
#define CUDA_ML_OPTIMISER_H_

#include <pthread.h>
#include "src/ml_optimiser.h"
#include "src/ml_model.h"
#include "src/parallel.h"
#include "src/exp_model.h"
#include "src/ctf.h"
#include "src/time.h"
#include "src/mask.h"
#include "src/healpix_sampling.h"

class MlOptimiserCUDA
{
public:


	/* Flag to indicate orientational (i.e. rotational AND translational) searches will be skipped */
	bool do_skip_align;

	/* Flag to indicate rotational searches will be skipped */
	bool do_skip_rotate;

	// Experimental metadata model
	Experiment mydata;

	// Current ML model
	MlModel mymodel;

	// HEALPix sampling object for coarse sampling
	HealpixSampling sampling;

	// Tabulated sin and cosine functions for shifts in Fourier space
	TabSine tab_sin;
	TabCosine tab_cos;

	// Calculate translated images on-the-fly
	bool do_shifts_onthefly;

	int iter;

	// Skip marginalisation in first iteration and use signal cross-product instead of Gaussian
	bool do_firstiter_cc;

	/// Always perform cross-correlation instead of marginalization
	bool do_always_cc;

	//  Use images only up to a certain resolution in the expectation step
	int coarse_size;

    // Array with pointers to the resolution of each point in a Fourier-space FFTW-like array
	MultidimArray<int> Mresol_fine, Mresol_coarse;

	// Multiplicative fdge factor for the sigma estimates
	double sigma2_fudge;

	// Flag whether to do CTF correction
	bool do_ctf_correction;

	// Flag whether current references are ctf corrected
	bool refs_are_ctf_corrected;

	// Flag whether to do group-wise intensity bfactor correction
	bool do_scale_correction;

	std::vector<MultidimArray<Complex> > global_fftshifts_ab_coarse, global_fftshifts_ab_current,
											global_fftshifts_ab2_coarse, global_fftshifts_ab2_current;

	int adaptive_oversampling;

	// Strict high-res limit in the expectation step
	double strict_highres_exp;

	/* Flag to indicate maximization step will be skipped: only data.star file will be written out */
	bool do_skip_maximization;

	// Flag whether to use maximum a posteriori (MAP) estimation
	bool do_map;

	MultidimArray<double> exp_metadata;

	// Current weighted sums
	MlWsumModel wsum_model;

	// Flag whether to do image-wise intensity-scale correction
	bool do_norm_correction;

	MlOptimiserCUDA(const MlOptimiser &parentOptimiser)
	{
		mydata = parentOptimiser.mydata;
		mymodel = parentOptimiser.mymodel;
		sampling = parentOptimiser.sampling;

		do_skip_align = parentOptimiser.do_skip_align;
		do_skip_rotate = parentOptimiser.do_skip_rotate;
		iter = parentOptimiser.iter;
		do_firstiter_cc = parentOptimiser.do_firstiter_cc;
		do_always_cc = parentOptimiser.do_always_cc;
		coarse_size = parentOptimiser.coarse_size;
		Mresol_fine = parentOptimiser.Mresol_fine;
		Mresol_coarse = parentOptimiser.Mresol_coarse;
		sigma2_fudge = parentOptimiser.sigma2_fudge;
		tab_sin = parentOptimiser.tab_sin;
		tab_cos = parentOptimiser.tab_cos;
		do_ctf_correction = parentOptimiser.do_ctf_correction;
		do_shifts_onthefly = parentOptimiser.do_shifts_onthefly;
		refs_are_ctf_corrected = parentOptimiser.refs_are_ctf_corrected;
		do_scale_correction = parentOptimiser.do_scale_correction;
		global_fftshifts_ab_coarse = parentOptimiser.global_fftshifts_ab_coarse;
		global_fftshifts_ab_current = parentOptimiser.global_fftshifts_ab_current;
		global_fftshifts_ab2_coarse = parentOptimiser.global_fftshifts_ab2_coarse;
		global_fftshifts_ab2_current = parentOptimiser.global_fftshifts_ab2_current;
		strict_highres_exp = parentOptimiser.strict_highres_exp;

		adaptive_oversampling = parentOptimiser.adaptive_oversampling;
		do_skip_maximization = parentOptimiser.do_skip_maximization;
		do_map = parentOptimiser.do_map;
		exp_metadata = parentOptimiser.exp_metadata;
		wsum_model = parentOptimiser.wsum_model;
		do_norm_correction = parentOptimiser.do_norm_correction;
	};

	void getAllSquaredDifferences(
			long int my_ori_particle, int exp_current_image_size,
			int exp_ipass, int exp_current_oversampling, int metadata_offset,
			int exp_idir_min, int exp_idir_max, int exp_ipsi_min, int exp_ipsi_max,
			int exp_itrans_min, int exp_itrans_max, int my_iclass_min, int my_iclass_max,
			std::vector<double> &exp_min_diff2,
			std::vector<double> &exp_highres_Xi2_imgs,
			std::vector<MultidimArray<Complex > > &exp_Fimgs,
			std::vector<MultidimArray<double> > &exp_Fctfs,
			MultidimArray<double> &exp_Mweight,
			MultidimArray<bool> &exp_Mcoarse_significant,
			std::vector<int> &exp_pointer_dir_nonzeroprior, std::vector<int> &exp_pointer_psi_nonzeroprior,
			std::vector<double> &exp_directions_prior, std::vector<double> &exp_psi_prior,
			std::vector<MultidimArray<Complex > > &exp_local_Fimgs_shifted,
			std::vector<MultidimArray<double> > &exp_local_Minvsigma2s,
			std::vector<MultidimArray<double> > &exp_local_Fctfs,
			std::vector<double> &exp_local_sqrtXi2
		);

	void storeWeightedSumsCUDA(long int my_ori_particle, int exp_current_image_size,
			int exp_current_oversampling, int metadata_offset,
			int exp_idir_min, int exp_idir_max, int exp_ipsi_min, int exp_ipsi_max,
			int exp_itrans_min, int exp_itrans_max, int exp_iclass_min, int exp_iclass_max,
			std::vector<double> &exp_min_diff2,
			std::vector<double> &exp_highres_Xi2_imgs,
			std::vector<MultidimArray<Complex > > &exp_Fimgs,
			std::vector<MultidimArray<Complex > > &exp_Fimgs_nomask,
			std::vector<MultidimArray<double> > &exp_Fctfs,
			std::vector<MultidimArray<double> > &exp_power_imgs,
			std::vector<Matrix1D<double> > &exp_old_offset,
			std::vector<Matrix1D<double> > &exp_prior,
			MultidimArray<double> &exp_Mweight,
			MultidimArray<bool> &exp_Mcoarse_significant,
			std::vector<double> &exp_significant_weight,
			std::vector<double> &exp_sum_weight,
			std::vector<double> &exp_max_weight,
			std::vector<int> &exp_pointer_dir_nonzeroprior, std::vector<int> &exp_pointer_psi_nonzeroprior,
			std::vector<double> &exp_directions_prior, std::vector<double> &exp_psi_prior,
			std::vector<MultidimArray<Complex > > &exp_local_Fimgs_shifted,
			std::vector<MultidimArray<Complex > > &exp_local_Fimgs_shifted_nomask,
			std::vector<MultidimArray<double> > &exp_local_Minvsigma2s,
			std::vector<MultidimArray<double> > &exp_local_Fctfs,
			std::vector<double> &exp_local_sqrtXi2);

	void storeWeightedSums(long int my_ori_particle, int exp_current_image_size,
			int exp_current_oversampling, int metadata_offset,
			int exp_idir_min, int exp_idir_max, int exp_ipsi_min, int exp_ipsi_max,
			int exp_itrans_min, int exp_itrans_max, int exp_iclass_min, int exp_iclass_max,
			std::vector<double> &exp_min_diff2,
			std::vector<double> &exp_highres_Xi2_imgs,
			std::vector<MultidimArray<Complex > > &exp_Fimgs,
			std::vector<MultidimArray<Complex > > &exp_Fimgs_nomask,
			std::vector<MultidimArray<double> > &exp_Fctfs,
			std::vector<MultidimArray<double> > &exp_power_imgs,
			std::vector<Matrix1D<double> > &exp_old_offset,
			std::vector<Matrix1D<double> > &exp_prior,
			MultidimArray<double> &exp_Mweight,
			MultidimArray<bool> &exp_Mcoarse_significant,
			std::vector<double> &exp_significant_weight,
			std::vector<double> &exp_sum_weight,
			std::vector<double> &exp_max_weight,
			std::vector<int> &exp_pointer_dir_nonzeroprior, std::vector<int> &exp_pointer_psi_nonzeroprior,
			std::vector<double> &exp_directions_prior, std::vector<double> &exp_psi_prior,
			std::vector<MultidimArray<Complex > > &exp_local_Fimgs_shifted,
			std::vector<MultidimArray<Complex > > &exp_local_Fimgs_shifted_nomask,
			std::vector<MultidimArray<double> > &exp_local_Minvsigma2s,
			std::vector<MultidimArray<double> > &exp_local_Fctfs,
			std::vector<double> &exp_local_sqrtXi2);

	void precalculateShiftedImagesCtfsAndInvSigma2s(
			bool do_also_unmasked,
			long int my_ori_particle, int exp_current_image_size, int exp_current_oversampling,
			int exp_itrans_min, int exp_itrans_max,
			std::vector<MultidimArray<Complex > > &exp_Fimgs,
			std::vector<MultidimArray<Complex > > &exp_Fimgs_nomask,
			std::vector<MultidimArray<double> > &exp_Fctfs,
			std::vector<MultidimArray<Complex > > &exp_local_Fimgs_shifted,
			std::vector<MultidimArray<Complex > > &exp_local_Fimgs_shifted_nomask,
			std::vector<MultidimArray<double> > &exp_local_Fctfs,
			std::vector<double> &exp_local_sqrtXi2,
			std::vector<MultidimArray<double> > &exp_local_Minvsigma2s
		);

	bool isSignificantAnyParticleAnyTranslation(
			long int iorient,
			int exp_itrans_min,
			int exp_itrans_max,
			MultidimArray<bool> &exp_Mcoarse_significant
		);
};

#endif
