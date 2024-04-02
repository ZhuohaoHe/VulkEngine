#pragma once

#include "Device.hpp"

#include <string>
#include <vector>

struct PipelineConfigInfo {};

class Pipeline {
public:
	Pipeline(
		Device& device,
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo
	 );
	~Pipeline() {}
	
	Pipeline(const Pipeline&) = delete;
	void operator=(const Pipeline&) = delete;
	
	static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height);
	
private:
	static std::vector<char> readFile(const std::string& path);
	
	void createGraphicsPipeline(
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo
	);
	
	void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
	
	Device& m_device_ref;
	VkPipeline m_graphicsPipeline;
	VkShaderModule m_vertShaderModule;
	VkShaderModule m_fragShaderModule;
};
