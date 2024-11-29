
#include "pipeline_manager.hpp"

#include <hydra/engine/hydra_context.hpp>

namespace neam::hydra
{
  void pipeline_manager::register_shader_reload_event(hydra_context& hctx, bool use_graphic_queue)
  {
    on_shaders_reloaded = hctx.shmgr.on_shaders_reloaded.add([this, &hctx, use_graphic_queue]()
    {
      cr::out().warn("pipeline_manager: recreating all the pipelines (caused by shader reload)");
      need_refresh = true;
      // vk::queue& queue = (use_graphic_queue ? hctx.gqueue : hctx.cqueue);
      //
      // refresh(vrd, queue);
      //
      // vk::fence fence { hctx.device };
      // queue.submit(hctx.dqe, fence);
      // vrd.postpone_destruction_inclusive(queue, std::move(fence));
      // hctx.vrd.append(std::move(vrd));
    });
  }
}
