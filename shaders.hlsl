#define PI 3.14159265359f

#if defined(D3D11)
#define CONSTANT_BUFFER(type, name, reg)                                       \
    cbuffer sm50_##name : register(reg)                                        \
    {                                                                          \
        type name;                                                             \
    }
#else
#define CONSTANT_BUFFER(type, name, reg)                                       \
    ConstantBuffer<type> name : register(reg)
#endif

struct constants
{
    uint vertex_offset;
    uint index_offset;
    uint material_id;
    uint texture_id;
    float4x4 global_transform;
};

struct frame_data
{
    float4x4 world_from_model;
    float4x4 clip_from_world;
    float3 camera_pos;
};

struct vertex
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 tex_coord;
    float4 color;
    uint4 joint_ids;
    float4 joint_weights;
};

struct material_properties
{
    uint has_texture;
    float alpha_cutoff;
    float metallic_factor;
    float roughness_factor;
    float3 emissive_factor;
    float4 base_color_factor;
    uint alpha_mode;
};

struct vs_out
{
    float4 position : SV_POSITION; // clip space
    float3 normal : NORMAL;        // world space
    float3 tangent : TANGENT;      // world space
    float3 bitangent : BITANGENT;  // world space
    float3 view_dir : VIEWDIR;     // world space (towards target)
    float2 tex_coord : TEXCOORD;   // texture space
    float4 color : COLOR;
};

struct ps_out
{
    float4 color : SV_TARGET;
};

// Shared Resources
#if defined(VULKAN)
[[vk::push_constant]]
#endif
CONSTANT_BUFFER(constants, per_draw_cb, b0);

// Vertex Shader Resources
CONSTANT_BUFFER(frame_data, per_frame_cb, b1);
StructuredBuffer<vertex> vertices_sb : register(t2);
StructuredBuffer<uint> indices_sb : register(t3);
StructuredBuffer<float4x4> joint_transforms_sb : register(t4);

// Pixel Shader Resources
StructuredBuffer<material_properties> material_properties_sb : register(t5);
#if defined(D3D12) || defined(VULKAN)
Texture2D textures[] : TEXTURE : register(t6, space1);
#else
Texture2D textures[4] : TEXTURE : register(t6);
#endif
SamplerState ss : SAMPLER : register(s0);

vs_out
vs(uint index_id : SV_VertexID)
{
    uint vertex_id = indices_sb[per_draw_cb.index_offset + index_id];
    vertex v = vertices_sb[per_draw_cb.vertex_offset + vertex_id];

    float joint_weight_sum = 0.0f;
    float4x4 skin_transform = 0.0f;
    for (uint i = 0; i < 4; i += 1)
    {
        skin_transform
            += v.joint_weights[i] * joint_transforms_sb[v.joint_ids[i]];
        joint_weight_sum += v.joint_weights[i];
    }
    float4x4 world_from_model
        = mul(per_frame_cb.world_from_model,
              joint_weight_sum > 0.0f ? skin_transform
                                      : per_draw_cb.global_transform);

    vs_out o;

    // NOTE: When multiplying the global transform, the w component must be
    // 1.0f for position vectors and 0.0f for direction vectors.
    o.position = mul(per_frame_cb.clip_from_world,
                     mul(world_from_model, float4(v.position, 1.0f)));

    // NOTE: Translation is ignored by casting to a 3x3 matrix.
    // NOTE: This assumes uniform scaling. For non-uniform scaling, use the
    // inverse transpose to undo the model matrix's scale transform but
    // preserve its rotation.
    o.normal = normalize(mul((float3x3)world_from_model, v.normal));
    o.tangent = normalize(mul((float3x3)world_from_model, v.tangent.xyz));

    // Reorthogonalize tangent with respect to normal using Gram-Schmidt.
    o.tangent = normalize(o.tangent - (dot(o.tangent, o.normal) * o.normal));

    o.bitangent = normalize(cross(o.normal, o.tangent) * v.tangent.w);

    o.view_dir = mul(world_from_model, float4(v.position, 1.0f)).xyz
                 - per_frame_cb.camera_pos;

    o.tex_coord = v.tex_coord;
    o.color = v.color;

    return o;
}

// Fresnel Reflectance using Schlick approximation
// The fresnel equation approximates the percentage of light reflected.
float3
fresnel_schlick(float v_dot_h, float3 f0)
{
    return f0 + ((1.0f - f0) * pow(1.0f - min(v_dot_h, 1.0f), 5.0f));
}

// Normal Distribution Function using GGX
// The normal distribution function (NDF) describes the orientation of
// microfacet normals, which determines the shape of highlights and changes how
// rough or smooth a surface appears.
float
ndf_ggx(float n_dot_h, float roughness)
{
    float alpha = pow(roughness, 2.0f);
    float alpha_sq = pow(alpha, 2.0f);
    float denom = PI * pow(pow(n_dot_h, 2.0f) * (alpha_sq - 1.0f) + 1.0f, 2.0f);

    return alpha_sq / denom;
}

// Schlick-GGX G1 function
float
g1_schlick_ggx(float n_dot_x, float k)
{
    return n_dot_x / (n_dot_x * (1.0f - k) + k);
}

// Geometry Term using Smith method
// The geometry term approximates the probability that light is occluded by
// microfacets.
float
geometry_smith(float n_dot_l, float n_dot_v, float roughness)
{
    float r = roughness + 1.0f;
    float k = pow(r, 2.0f) / 8.0f;

    return g1_schlick_ggx(n_dot_l, k) * g1_schlick_ggx(n_dot_v, k);
}

// NOTE: hk_alpha_mode values: HK_AM_OPAQUE (0), HK_AM_MASK (1), HK_AM_BLEND (2)
float4
get_base_color(uint tex_offset,
               float2 tex_coord,
               float4 vertex_color,
               material_properties mp)
{
    float4 base_color = vertex_color * mp.base_color_factor;
    if (mp.has_texture & (1u << 0))
    {
        base_color *= textures[tex_offset + 0].Sample(ss, tex_coord);
    }

    if (mp.alpha_mode == 1 && base_color.a < mp.alpha_cutoff)
    {
        discard;
    }

    base_color.a = (mp.alpha_mode == 2) ? base_color.a : 1.0f;

    return base_color;
}

float
get_metallic(uint tex_offset, float2 tex_coord, material_properties mp)
{
    float metallic = mp.metallic_factor;
    if (mp.has_texture & (1u << 1))
    {
        metallic *= textures[tex_offset + 1].Sample(ss, tex_coord).b;
    }

    return metallic;
}

float
get_roughness(uint tex_offset, float2 tex_coord, material_properties mp)
{
    float roughness = mp.roughness_factor;
    if (mp.has_texture & (1u << 1))
    {
        roughness *= textures[tex_offset + 1].Sample(ss, tex_coord).g;
    }

    return roughness;
}

float3
get_normal(uint tex_offset,
           float2 tex_coord,
           float3 normal,
           float3 tangent,
           float3 bitangent,
           material_properties mp)
{
    if (mp.has_texture & (1u << 2))
    {
        // NOTE: Normals are remapped from [0, 1] to [-1, 1] and transformed
        // from tangent space to world space.
        // NOTE: glTF normal maps are +Y/Green-up. green on bottom = hole,
        // green on top = bump
        float3 tex_normal = textures[tex_offset + 2].Sample(ss, tex_coord).rgb;
        tex_normal = (tex_normal * 2.0f) - 1.0f;
        float3x3 tbn = transpose(float3x3(tangent, bitangent, normal));
        return mul(tbn, tex_normal);
    }
    else
    {
        // If no normal texture, use world-space vertex normal.
        return normal;
    }
}

float3
get_emissive(uint tex_offset, float2 tex_coord, material_properties mp)
{
    float3 emissive = mp.emissive_factor;
    if (mp.has_texture & (1u << 3))
    {
        emissive *= textures[tex_offset + 3].Sample(ss, tex_coord).rgb;
    }

    return emissive;
}

ps_out
ps(vs_out i)
{
    material_properties mp = material_properties_sb[per_draw_cb.material_id];

    // Sample textures.
#if defined(D3D12) || defined(VULKAN)
    uint tex_offset = per_draw_cb.texture_id;
#else
    uint tex_offset = 0;
#endif
    float4 base_color = get_base_color(tex_offset, i.tex_coord, i.color, mp);
    float metallic = get_metallic(tex_offset, i.tex_coord, mp);
    float roughness = get_roughness(tex_offset, i.tex_coord, mp);
    float3 normal = get_normal(tex_offset,
                               i.tex_coord,
                               i.normal,
                               i.tangent,
                               i.bitangent,
                               mp);
    float3 emissive = get_emissive(tex_offset, i.tex_coord, mp);

    // Microfacet BRDF using Cook-Torrance model
    // The bidirectional reflective distribution function (BRDF) is the
    // approximation of the light reflected off a surface given its material
    // properties.
    // NOTE: `f0` is the base reflectivity when looking directly at the surface
    // (i.e. 0 degree angle between `n` and `v`).
    float3 n = normalize(normal);
    float3 l = normalize(-i.view_dir); // NOTE: `light_dir` for manual control
    float3 v = normalize(-i.view_dir);
    float3 h = normalize(l + v);
    float n_dot_v = max(0.0f, dot(n, v));
    float n_dot_l = max(0.0f, dot(n, l));
    float n_dot_h = max(0.0f, dot(n, h));
    float v_dot_h = max(0.0f, dot(v, h));
    float3 dielectric_fresnel_factor = float3(0.04f, 0.04f, 0.04f);
    float3 f0 = lerp(dielectric_fresnel_factor, base_color.rgb, metallic);
    float3 f = fresnel_schlick(v_dot_h, f0);
    float d = ndf_ggx(n_dot_h, roughness);
    float g = geometry_smith(n_dot_l, n_dot_v, roughness);
    float3 refracted_light
        = float3(1.0f, 1.0f, 1.0f) - f; // all non-reflected light
    float dielectric = 1.0f - metallic; // diffuse BRDF = 0.0f for metals
    float3 diffuse_brdf = (base_color.rgb * refracted_light * dielectric) / PI;
    float3 specular_brdf = (f * d * g) / max(0.0001f, 4.0f * n_dot_l * n_dot_v);
    float3 brdf = diffuse_brdf + specular_brdf;

    float3 light_color = float3(1.0f, 1.0f, 1.0f);
    float attenuation = 1.0f; // attenuation = 1.0f for directional lights
    float3 radiance = light_color * attenuation;

    float3 ambient = base_color.rgb * float3(0.35f, 0.35f, 0.35f);

    float3 color = (brdf * radiance * n_dot_l) + ambient + emissive;

    ps_out o;
    o.color = float4(color, base_color.a);
    return o;
}
