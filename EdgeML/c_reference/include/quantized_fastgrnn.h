// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef __QUANTIZED_FASTGRNN_H__
#define __QUANTIZED_FASTGRNN_H__

#define ERR_PRECOMP_NOT_INIT -1
#define ERR_TEMPLRW_NOT_INIT -2
#define ERR_TEMPLRU_NOT_INIT -3
#define ERR_NORMFEATURES_NOT_INIT -4

#include "quantized_utils.h"

/**
 * @brief Model parameters for low-rank FastGRNN
 * @var       mean         pointer to mean of input vector for normalization, size inputDims
 * @var       stdDev       pointer to standard dev of input for normalization, size inputDims * steps
 * @var       W1           pointer to first low-rank component of W
 * @var       W2           pointer to second low-rank component of W
 * @var       wRank        rank of W matrix
 * @var       U1           pointer to first low-rank component of U
 * @var       U2           pointer to second low-rank component of U
 * @var       uRank        rank of U matrix
 * @var       Bg           pointer to bias for Sigmoid
 * @var       Bh           pointer to bias for TanH
 * @var       sigmoid_zeta first weight parameter for update from input from next step
 * @var       sigmoid_nu   second weight parameter for update from input from next step
 */
typedef struct Q15_FastGRNN_LR_Params {
  const Q15_T* mean;
  const Q15_T* stdDev;
  const Q15_T* W1;
  const Q15_T* W2;
  ITER_T wRank;
  const Q15_T* U1;
  const Q15_T* U2;
  ITER_T uRank;
  const Q15_T* Bg;
  const Q15_T* Bh;
  Q15_T sigmoid_zeta;
  Q15_T sigmoid_nu;
} Q15_FastGRNN_LR_Params;

/**
 * @brief Model scales and flags for different inputs. The naming convention
 * follows three basic rules:
 * 1) If the variable for which the scale is associated is used only once, name
 * the scale after the matrix.
 * 2) If the variable for which the scale is associated is used more than once,
 * name the scale after the matrix, the operation it is being used under and
 * the matrix it is being operated with.
 * 3) For flag variables, simply name them after the operation for which they
 * are intended.
 */
typedef struct Q15_FastGRNN_LR_Scales {
  SCALE_T input;
  SCALE_T mean;
  SCALE_T meanSub;
  SCALE_T stdDev;
  SCALE_T normFeaturesHDStdDev;
  SCALE_T w1;
  SCALE_T normFeaturesMVW1;
  SCALE_T mVW1Out;
  SCALE_T w2;
  SCALE_T tempLRW;
  SCALE_T mVW2Out;
  SCALE_T u1;
  SCALE_T hiddenStateMVU1;
  SCALE_T mVU1Out;
  SCALE_T u2;
  SCALE_T tempLRU;
  SCALE_T mVU2Out;
  SCALE_T mV2AddMV4;
  SCALE_T mV4AddMV2;
  SCALE_T mV2AddMV4Out;
  SCALE_T mV2AddMV4Demote;
  SCALE_T pC1AddBg;
  SCALE_T bg;
  SCALE_T pC1AddBgOut;
  SCALE_T pC1AddBgDemote;
  SCALE_T sigmoidScaleIn;
  SCALE_T sigmoidScaleOut;
  SCALE_T pC1AddBh;
  SCALE_T bh;
  SCALE_T pC1AddBhOut;
  SCALE_T pC1AddBhDemote;
  SCALE_T tanhScaleIn;
  SCALE_T tanhScaleOut;
  SCALE_T gateHDHiddenState;
  SCALE_T hiddenStateHDGate;
  SCALE_T qOneScale;
  SCALE_T qOneSubGate;
  SCALE_T qOneSubGateOut;
  SCALE_T sigmoidZeta;
  SCALE_T sigmoidZetaMulQOneSubGate;
  SCALE_T sigmoidNu;
  SCALE_T sigmoidNuAddQOneSubGate;
  SCALE_T sigmoidNuAddQOneSubGateOut;
  SCALE_T sigmoidNuAddQOneSubGateHDUpdate;
  SCALE_T updateHDSigmoidNuAddQOneSubGate;
  SCALE_T pC3AddPC1;
  SCALE_T pC1AddPC3;
  SCALE_T hiddenStateOut;
  SCALE_T hiddenStateDemote;
  Q15_T sigmoidLimit;
  Q15_T div;
  Q15_T add;
  Q15_T qOne;
  ITER_T useTableSigmoid;
  ITER_T useTableTanH;
} Q15_FastGRNN_LR_Scales;

/**
* @brief Buffers required for computation of low-rank FastGRNN
* @var   preComp1     pointer to buffer space, must be initialized to atleast hiddenDims size
* @var   preComp2     pointer to buffer space, must be initialized to atleast hiddenDims size
* @var   preComp3     pointer to buffer space, must be initialized to atleast hiddenDims size
* @var   tempLRW      pointer to buffer space, must be initialized to atleast wRank size
* @var   tempLRU      pointer to buffer space, must be initialized to atleast uRank size
* @var   normFeatures pointer to buffer space, must be initialized to atleast inputDims size
*/
typedef struct Q15_FastGRNN_LR_Buffers {
  Q15_T* preComp1;
  Q15_T* preComp2;
  Q15_T* preComp3;
  Q15_T* tempLRW;
  Q15_T* tempLRU;
  Q15_T* normFeatures;
} Q15_FastGRNN_LR_Buffers;

/**
 * @brief Multi-step updates of a FastGRNN cell with low rank W, U (W = W1 * W2; U = U1 * U2)
 * @param[in,out]   hiddenState  pointer to initial hidden state and output hidden state
 * @param[in]       hiddenDims   dimension of hidden state of the FastGRNN cell
 * @param[in]       input        pointer to concatenated input vectors for all steps, size inputDims * steps
 * @param[in]       inputDims    dimension of input vector for each step
 * @param[in]       steps        number of steps of FastGRNN cell
 * @param[in]       params       pointer to model parameter
 * @param[in]       buffers      pointer to buffer spaces
 * @param[in]       scales       pointer to model scales
 * @param[in]       backward     direction of the pass, 0 for forward, 1 for backward
 * @param[in]       normalize    apply mean-var normalization, 0 for no, 1 for yes
 * @return     The function returns <code>0</code> on success
 *             <code>ERR_PRECOMP_NOT_INIT</code> if preComp1 not allocated
 *             <code>ERR_PRECOMP_NOT_INIT</code> if preComp2 not allocated
 *             <code>ERR_PRECOMP_NOT_INIT</code> if preComp3 not allocated
 *             <code>ERR_TEMPLRW_NOT_INIT</code> if tempLRW not allocated
 *             <code>ERR_TEMPLRU_NOT_INIT</code> if tempLRU not allocated
 *             <code>ERR_NORMFEAT_NOT_INIT</code> if normFeatures not allocated
*/
int q15_fastgrnn_lr(Q15_T* const hiddenState, ITER_T hiddenDims,
                    const Q15_T* const input, ITER_T inputDims, ITER_T steps,
                    const void* params, void* buffers, const void* scales,
                    int backward, int normalize);

/**
 * @brief Model paramters for FastGRNN
 * Note: Wids, Wvals, Uids, Uvals can be set to NULL for non-sparse mode operation
 * @var       mean         pointer to mean of input vector for normalization, size inputDims
 * @var       stdDev       pointer to standard dev of input for normalization, size inputDims * steps
 * @var       W            pointer to W matrix
 * @var       Wids         pointer to the matrix storing row-indices of non-zero elements of W
 * @var       Wvals        pointer to the matrix storing the non-zero elements of W
 * @var       U            pointer U matrix
 * @var       Uids         pointer to the matrix storing row-indices of non-zero elements of U
 * @var       Uvals        pointer to the matrix storing the non-zero elements of U
 * @var       Bg           pointer to bias for Sigmoid
 * @var       Bh           pointer to bias for TanH
 * @var       sigmoid_zeta first weight parameter for update from input from next step
 * @var       sigmoid_nu   second weight parameter for update from input from next step
 */
typedef struct Q15_FastGRNN_Params {
  const Q15_T* mean;
  const Q15_T* stdDev;
  const Q15_T* W;
  const ITER_T* Wids;
  const Q15_T* Wvals;
  const Q15_T* U;
  const ITER_T* Uids;
  const Q15_T* Uvals;
  const Q15_T* Bg;
  const Q15_T* Bh;
  Q15_T sigmoid_zeta;
  Q15_T sigmoid_nu;
} Q15_FastGRNN_Params;

typedef struct Q7xQ15_FastGRNN_Params {
  const Q7_T* mean;
  const Q7_T* stdDev;
  const Q15_T* W;
  const ITER_T* Wids;
  const Q15_T* Wvals;
  const Q15_T* U;
  const ITER_T* Uids;
  const Q15_T* Uvals;
  const Q15_T* Bg;
  const Q15_T* Bh;
  Q15_T sigmoid_zeta;
  Q15_T sigmoid_nu;
} Q7xQ15_FastGRNN_Params;

/**
 * @brief Model scales and flags for different inputs. The naming convention
 * follows three basic rules:
 * 1) If the variable for which the scale is associated is used only once, name
 * the scale after the matrix.
 * 2) If the variable for which the scale is associated is used more than once,
 * name the scale after the matrix, the operation it is being used under and
 * the matrix it is being operated with.
 * 3) For flag variables, simply name them after the operation for which they
 * are intended.
 */
typedef struct Q15_FastGRNN_Scales {
  SCALE_T input;
  SCALE_T mean;
  SCALE_T meanSub;
  SCALE_T stdDev;
  SCALE_T normFeaturesHDStdDev;
  SCALE_T w;
  SCALE_T normFeaturesMVW;
  SCALE_T mVWOut;
  SCALE_T u;
  SCALE_T hiddenStateMVU;
  SCALE_T mVUOut;
  SCALE_T mV1AddMV2;
  SCALE_T mV2AddMV1;
  SCALE_T mV1AddMV2Out;
  SCALE_T mV1AddMV2Demote;
  SCALE_T pC1AddBg;
  SCALE_T bg;
  SCALE_T pC1AddBgOut;
  SCALE_T pC1AddBgDemote;
  SCALE_T sigmoidScaleIn;
  SCALE_T sigmoidScaleOut;
  SCALE_T pC1AddBh;
  SCALE_T bh;
  SCALE_T pC1AddBhOut;
  SCALE_T pC1AddBhDemote;
  SCALE_T tanhScaleIn;
  SCALE_T tanhScaleOut;
  SCALE_T gateHDHiddenState;
  SCALE_T hiddenStateHDGate;
  SCALE_T qOneScale;
  SCALE_T qOneSubGate;
  SCALE_T qOneSubGateOut;
  SCALE_T sigmoidZeta;
  SCALE_T sigmoidZetaMulQOneSubGate;
  SCALE_T sigmoidNu;
  SCALE_T sigmoidNuAddQOneSubGate;
  SCALE_T sigmoidNuAddQOneSubGateOut;
  SCALE_T sigmoidNuAddQOneSubGateHDUpdate;
  SCALE_T updateHDSigmoidNuAddQOneSubGate;
  SCALE_T pC3AddPC1;
  SCALE_T pC1AddPC3;
  SCALE_T hiddenStateOut;
  SCALE_T hiddenStateDemote;
  Q15_T div;
  Q15_T add;
  Q15_T sigmoidLimit;
  Q15_T qOne;
  ITER_T useTableSigmoid;
  ITER_T useTableTanH;
} Q15_FastGRNN_Scales;

/**
* @brief Buffers required for computation of FastGRNN
* @var   preComp1     pointer to buffer space, must be initialized to atleast hiddenDims size
* @var   preComp2     pointer to buffer space, must be initialized to atleast hiddenDims size
* @var   preComp3     pointer to buffer space, must be initialized to atleast hiddenDims size
* @var   normFeatures pointer to buffer space, must be initialized to atleast inputDims size
*/
typedef struct Q15_FastGRNN_Buffers {
  Q15_T* preComp1;
  Q15_T* preComp2;
  Q15_T* preComp3;
  Q15_T* normFeatures;
} Q15_FastGRNN_Buffers;

typedef struct Q7xQ15_FastGRNN_Buffers {
  Q15_T* preComp1;
  Q15_T* preComp2;
  Q15_T* preComp3;
  Q7_T* normFeatures;
} Q7xQ15_FastGRNN_Buffers;

/**
 * @brief Multi-step updates of a FastGRNN cell
 * @param[in,out]   hiddenState  pointer to initial hidden state and output hidden state
 * @param[in]       hiddenDims   dimension of hidden state of the FastGRNN cell
 * @param[in]       input        pointer to concatenated input vectors for all steps, size inputDims * steps
 * @param[in]       inputDims    dimension of input vector for each step
 * @param[in]       steps        number of steps of FastGRNN cell
 * @param[in]       params       pointer to model parameter
 * @param[in]       buffers      pointer to buffer spaces
 * @param[in]       scales       pointer to model scales
 * @param[in]       backward     direction of the pass, 0 for forward, 1 for backward
 * @param[in]       normalize    apply mean-var normalization, 0 for no, 1 for yes
 * @return     The function returns <code>0</code> on success
 *             <code>ERR_PRECOMP_NOT_INIT</code> if preComp1 not allocated
 *             <code>ERR_PRECOMP_NOT_INIT</code> if preComp2 not allocated
 *             <code>ERR_PRECOMP_NOT_INIT</code> if preComp3 not allocated
 *             <code>ERR_NORMFEAT_NOT_INIT</code> if normFeatures not allocated
 * @example         Please refer the file: c_reference/tests/fastgrnn/test_quantized_fastgrnn.c
 */
int q7xq15_q15_fastgrnn(Q15_T* const hiddenState, ITER_T hiddenDims,
  const Q7_T* const input, ITER_T inputDims, ITER_T steps, const void* params,
  void* buffers, const void* scales, int backward, int normalize);
int q15_fastgrnn(Q15_T* const hiddenState, ITER_T hiddenDims,
  const Q15_T* const input, ITER_T inputDims, ITER_T steps, const void* params,
  void* buffers, const void* scales, int backward, int normalize);

#endif
