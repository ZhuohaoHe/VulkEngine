#include "Application.hpp"
#include <iostream>
#include <filesystem>
#include <stdexcept>

Application::Application() {
	loadModel();
	createPipelineLayout();
	recreateSwapChain();
	createCommandBuffers();
}

Application::~Application() {
	vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
}

void Application::run() {
	while (!m_window.shouldClose()) {
		glfwPollEvents();
		drawFrame();
		
		vkDeviceWaitIdle(m_device.device()); // fix: All submitted commands that refer to pipeline must have completed execution
	}
}

void Application::loadModel() {
	std::vector<Model::Vertex> vertices {
		{{0.0, -0.5}, {1.0f, 0.0f, 0.0f}},
		{{0.5, 0.5}, {0.0f, 1.0f, 0.0f}},
		{{-0.5, 0.5}, {0.0f, 0.0f, 1.0f}}
	};
	
	m_model = std::make_unique<Model>(m_device, vertices);
}

void Application::createPipelineLayout() {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void Application::createPipeline() {
	auto pipelineConfig = Pipeline::defaultPipelineConfigInfo(m_swapChain->width(), m_swapChain->height());
	pipelineConfig.renderPass = m_swapChain->getRenderPass();
	pipelineConfig.pipelineLayout = m_pipelineLayout;
	m_pipeline = std::make_unique<Pipeline>(
	   m_device,
	   "shaders/simple_shader.vert.spv",
	   "shaders/simple_shader.frag.spv",
	   pipelineConfig
   );
}

void Application::recreateSwapChain() {
	auto extent = m_window.getExtent();
	while (extent.width == 0 || extent.height == 0) {
		// wait for minimization
		extent = m_window.getExtent();
		glfwWaitEvents();
	}
	
	vkDeviceWaitIdle(m_device.device()); // make sure the swap chain is no longer been used.
	m_swapChain = nullptr; // when mouse release, the program crush
	m_swapChain = std::make_unique<SwapChain>(m_device, extent);
	createPipeline();
}

void Application::createCommandBuffers(){
	m_commandBuffers.resize(m_swapChain->imageCount());
	
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_device.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
	
	if(vkAllocateCommandBuffers(m_device.device(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void Application::recordCommandBuffer(int imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	
	if (vkBeginCommandBuffer(m_commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_swapChain->getRenderPass();
	renderPassInfo.framebuffer = m_swapChain->getFrameBuffer(imageIndex);
	
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = m_swapChain->getSwapChainExtent();
	
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
	clearValues[1].depthStencil = {1.0f, 0};
	
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	
	vkCmdBeginRenderPass(m_commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	
	m_pipeline->bind(m_commandBuffers[imageIndex]);
	m_model->bind(m_commandBuffers[imageIndex]);
	m_model->draw(m_commandBuffers[imageIndex]);
	
	vkCmdEndRenderPass(m_commandBuffers[imageIndex]);
	if (vkEndCommandBuffer(m_commandBuffers[imageIndex]) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void Application::drawFrame() {
	uint32_t imageIndex;
	auto result = m_swapChain->acquireNextImage(&imageIndex);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	
	recordCommandBuffer(imageIndex);
	result = m_swapChain->submitCommandBuffers(&m_commandBuffers[imageIndex], &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.wasWindowResize()) {
		m_window.resetWindowResizedFlag();
		recreateSwapChain();
		return;
	}
	
	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
}

