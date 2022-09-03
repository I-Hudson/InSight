#include "Common.hlsl"

// Define all our textures needed

[[vk::combinedImageSampler]][[vk::binding(0, 1)]]
Texture2D<float4> GBuffer_Colour : register(t0);
[[vk::combinedImageSampler]][[vk::binding(0, 1)]]
SamplerState GBuffer_Colour_Sampler : register(s0);

[[vk::combinedImageSampler]][[vk::binding(1, 1)]]
Texture2D<float4> GBuffer_World_Normal: register(t1);
[[vk::combinedImageSampler]][[vk::binding(1, 1)]]
SamplerState GBuffer_World_Normal_Sampler : register(s1);

[[vk::combinedImageSampler]][[vk::binding(2, 1)]]
Texture2D<float4> GBuffer_World_Position : register(t2);
[[vk::combinedImageSampler]][[vk::binding(2, 1)]]
SamplerState GBuffer_World_Position_Sampler : register(s2);

[[vk::combinedImageSampler]][[vk::binding(3, 1)]]
Texture2D<float4> GBuffer_Shadow : register(t3);
[[vk::combinedImageSampler]][[vk::binding(3, 1)]]
SamplerState GBuffer_Shadow_Sampler : register(s3);

[[vk::binding(4, 1)]]
Texture2DArray<float> Cascade_Shadow : register(t4);
[[vk::binding(5, 1)]]
SamplerComparisonState Cascade_Shadow_Sampler : register(s4);


struct PushConstant
{
	int Output_Texture;
};
[[vk::push_constant]] PushConstant push_constant;

struct VertexOutput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

VertexOutput VSMain(uint id : SV_VertexID)
{
	VertexOutput o;
	o.UV = float2((id << 1) & 2, id & 2);
	o.Position = float4(o.UV * float2(2, -2) + float2(-1, 1), 0, 1);
	return o;
}

float Get_Normal_Dot_Light_Direction(float3 surface_normal, float3 light_direction)
{
	return saturate(dot(surface_normal, light_direction));
}

void Apply_Bias(inout float3 position, float3 surface_normal, float3 light_direction, float bias_mul = 1.0)
{
    //// Receiver plane bias (slope scaled basically)
    //float3 du                   = ddx(position);
    //float3 dv                   = ddy(position);
    //float2 receiver_plane_bias  = mul(transpose(float2x2(du.xy, dv.xy)), float2(du.z, dv.z));
    
    //// Static depth biasing to make up for incorrect fractional sampling on the shadow map grid
    //float sampling_error = min(2.0f * dot(g_shadow_texel_size, abs(receiver_plane_bias)), 0.01f);

    // Scale down as the user is interacting with much bigger, non-fractional values (just a UX approach)
    float fixed_factor = 0.0001f;
    
	float n_dot_l = Get_Normal_Dot_Light_Direction(surface_normal, light_direction);

    // Slope scaling
    float slope_factor = (1.0f - saturate(n_dot_l));

    // Apply bias
    position.z += fixed_factor * slope_factor * bias_mul;
}

float3 bias_normal_offset(float3 n_dot_l, float normal_bias, float2 shadow_resolution, float3 normal)
{
	float texel_size = 1.0 / shadow_resolution;
    return normal * (1.0f - saturate(n_dot_l)) * normal_bias * texel_size * 10;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{	
	float4 colour 			= GBuffer_Colour.Sample(GBuffer_Colour_Sampler, input.UV);
	float4 world_normal 	= GBuffer_World_Normal.Sample(GBuffer_World_Normal_Sampler, input.UV);
	float4 world_position 	= GBuffer_World_Position.Sample(GBuffer_World_Position_Sampler, input.UV);
	float gbuffer_depth 	= GBuffer_Shadow.Sample(GBuffer_Shadow_Sampler, input.UV).r;

	float3 reconstruct_world_position = reconstruct_position(input.UV, gbuffer_depth, Main_Camera_Projection_View_Inverted);

	float3 world_pos 		= world_position.xyz / world_position.w; 
	float4 position_view_space = float4(world_to_ndc(world_pos, Main_Camera_View), 1.0);
	float4 shadow = 0;
	
    for (uint cascade = 0; cascade < 2; cascade++)
    {
		Shadow_Camera shadow_camera = shadow_cameras[cascade];
		// Project into light space
		float4x4 shadow_space_matrix = shadow_camera.Shadow_Camera_ProjView;
        float3 shadow_pos_ndc = world_to_ndc(reconstruct_world_position, shadow_space_matrix);
		float2 shadow_uv = ndc_to_uv(shadow_pos_ndc);
		// Ensure not out of bound
    	if (is_saturated(shadow_uv))
    	{
 			//Apply_Bias(shadow_pos_ndc, world_normal.xyz, shadow_camera.Shadow_Light_Direction, cascade + 1);

			float shadow_sample = 
			Cascade_Shadow.SampleCmpLevelZero(Cascade_Shadow_Sampler
			, float3(shadow_uv.x, shadow_uv.y, cascade)
			, shadow_pos_ndc.z).r;

			shadow = float4(shadow_sample, shadow_sample, shadow_sample, 1.0);
			break;
		}
	}
	float4 result;

	if(push_constant.Output_Texture == 0)
	{
		result = colour;
	}
	else if(push_constant.Output_Texture == 1)
	{
		world_normal = (world_normal + 1) / 2; //Remap normal to 0-1.
		result = world_normal;
	}
	else if(push_constant.Output_Texture == 2)
	{
		result = world_position;
	}
	else if(push_constant.Output_Texture == 3)
	{
		result = shadow;
	}
	else if(push_constant.Output_Texture == 4)
	{
		result = float4(0, 0, position_view_space.z, 1);
	}
	else if(push_constant.Output_Texture == 5)
	{
		result = float4(gbuffer_depth, gbuffer_depth, gbuffer_depth, 1);
	}
		else if(push_constant.Output_Texture == 6)
	{
		result = float4(world_to_ndc(reconstruct_world_position, Main_Camera_Proj_View), 1);
	}
	return result;
}