//
//  Multigrid.h
//  Vortex2D
//

#ifndef Vortex2D_Multigrid_h
#define Vortex2D_Multigrid_h

#include <Vortex2D/Engine/LevelSet.h>
#include <Vortex2D/Engine/LinearSolver/GaussSeidel.h>
#include <Vortex2D/Engine/LinearSolver/Jacobi.h>
#include <Vortex2D/Engine/LinearSolver/LinearSolver.h>
#include <Vortex2D/Engine/LinearSolver/Preconditioner.h>
#include <Vortex2D/Engine/LinearSolver/Transfer.h>
#include <Vortex2D/Renderer/CommandBuffer.h>
#include <Vortex2D/Renderer/Texture.h>
#include <Vortex2D/Renderer/Timer.h>
#include <Vortex2D/Renderer/Work.h>

namespace Vortex2D
{
namespace Fluid
{
/**
 * @brief Contains the sizes of the multigrid hierarchy.
 */
class Depth
{
public:
  /**
   * @brief Initialize with the finest size.
   * @param size the base size.
   */
  Depth(const glm::ivec2& size);

  /**
   * @brief The calculated depth of the multigrid.
   * @return the depth.
   */
  int GetMaxDepth() const;

  /**
   * @brief Gets the depth for a given level
   * @param i the level
   * @return the size
   */
  glm::ivec2 GetDepthSize(std::size_t i) const;

private:
  std::vector<glm::ivec2> mDepths;
};

class Pressure;

/**
 * @brief Multigrid preconditioner. It creates a hierarchy of twice as small set
 * of linear equations. It applies a few iterations of jacobi on each level and
 * transfers the error on the level above. It then copies the error down, adds
 * to the current solution and apply a few more iterations of jacobi.
 */
class Multigrid : public LinearSolver, public Preconditioner
{
public:
  enum class SmootherSolver
  {
    Jacobi,
    GaussSeidel,
  };

  /**
   * @brief Initialize multigrid for given size and delta.
   * @param device vulkan device
   * @param size of the linear equations
   * @param delta timestep delta
   */
  VORTEX2D_API Multigrid(const Renderer::Device& device,
                         const glm::ivec2& size,
                         float delta,
                         int numSmoothingIterations = 3,
                         SmootherSolver smoother = SmootherSolver::Jacobi);

  VORTEX2D_API ~Multigrid() override;

  VORTEX2D_API void Bind(Renderer::GenericBuffer& d,
                         Renderer::GenericBuffer& l,
                         Renderer::GenericBuffer& b,
                         Renderer::GenericBuffer& x) override;

  /**
   * @brief Bind the level sets from which the hierarchy is built.
   * @param pressure The current linear equations
   * @param solidPhi the solid level set
   * @param liquidPhi the liquid level set
   */
  VORTEX2D_API void BuildHierarchiesBind(Pressure& pressure,
                                         Renderer::Texture& solidPhi,
                                         Renderer::Texture& liquidPhi);

  /**
   * @brief Computes the hierarchy to be used by the multigrid. Asynchronous
   * operation.
   */
  VORTEX2D_API void BuildHierarchies();

  void Record(vk::CommandBuffer commandBuffer) override;

  void BindRigidbody(float delta, Renderer::GenericBuffer& d, RigidBody& rigidBody) override;

  /**
   * @brief Solves the linear equations
   * @param params solver iteration/error parameters
   * @param rigidBodies rigidbody to include in solver's matrix
   */
  VORTEX2D_API void Solve(Parameters& params,
                          const std::vector<RigidBody*>& rigidBodies = {}) override;

  /**
   * @return the max error
   */
  VORTEX2D_API float GetError() override;

private:
  void Smoother(vk::CommandBuffer commandBuffer, int n);

  void RecursiveBind(Pressure& pressure, std::size_t depth);

  void RecordVCycle(vk::CommandBuffer commandBuffer, int depth);
  void RecordFullCycle(vk::CommandBuffer commandBuffer);

  const Renderer::Device& mDevice;
  Depth mDepth;
  float mDelta;
  int mNumSmoothingIterations;

  Renderer::Work mResidualWork;
  std::vector<Renderer::Work::Bound> mResidualWorkBound;

  Transfer mTransfer;

  Renderer::GenericBuffer* mPressure = nullptr;

  // mDatas[0]  is level 1
  std::vector<LinearSolver::Data> mDatas;

  // mResiduals[0] is level 0
  std::vector<Renderer::Buffer<float>> mResiduals;

  Renderer::Work mPhiScaleWork;
  std::vector<Renderer::Work::Bound> mSolidPhiScaleWorkBound;
  std::vector<Renderer::Work::Bound> mLiquidPhiScaleWorkBound;

  // mSolidPhis[0] and mLiquidPhis[0] is level 1
  std::vector<LevelSet> mSolidPhis;
  std::vector<LevelSet> mLiquidPhis;

  std::vector<Renderer::Work::Bound> mMatrixBuildBound;

  // mSmoothers[0] is level 0
  std::vector<std::unique_ptr<Preconditioner>> mSmoothers;
  LocalGaussSeidel mSmoother;

  Renderer::CommandBuffer mBuildHierarchies;
  Renderer::CommandBuffer mFullCycleSolver, mVCycleSolver;

  LinearSolver::Error mError;
};

}  // namespace Fluid
}  // namespace Vortex2D

#endif
