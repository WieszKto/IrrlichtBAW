#ifndef _EXTRA_CRAP_INCLUDED_
#define _EXTRA_CRAP_INCLUDED_

#include "irrlicht.h"

#include "../../ext/RadeonRays/RadeonRays.h"
// pesky leaking defines
#undef PI


class Renderer : public irr::core::IReferenceCounted, public irr::core::InterfaceUnmovable
{
    public:
		struct alignas(16) SLight
		{
			enum E_TYPE : uint32_t
			{
				ET_CONSTANT,
				ET_CUBE,
				ET_ELLIPSOID,
				ET_CYLINDER,
				ET_RECTANGLE,
				ET_DISK,
				ET_TRIANGLE,
				ET_COUNT
			};

			SLight() : type(ET_COUNT) {}
			SLight(const SLight& other) : type(other.type)
			{
				std::copy(other.strengthFactor,other.strengthFactor+3u,strengthFactor);
				if (type == ET_TRIANGLE)
					std::copy(other.triangle.vertices, other.triangle.vertices + 3u, triangle.vertices);
				else
					transform = other.transform;
			}
			~SLight() {}

			void setFactor(const irr::core::vectorSIMDf& factor)
			{
				for (auto i=0u; i<3u; i++)
					strengthFactor[i] = factor[i];
			}

			//! This is according to Rec.709 colorspace
			inline float getFactorLuminosity()
			{
				float rec709LumaCoeffs[] = {0.2126f, 0.7152, 0.0722};

				//! TODO: More color spaces!
				float* colorSpaceLumaCoeffs = rec709LumaCoeffs;

				float luma = strengthFactor[0] * colorSpaceLumaCoeffs[0];
				luma += strengthFactor[1] * colorSpaceLumaCoeffs[1];
				luma += strengthFactor[2] * colorSpaceLumaCoeffs[2];
				return luma;
			}

			inline float computeFlux(float triangulizationArea) // also known as lumens
			{
				const auto unitHemisphereArea = 2.f*irr::core::PI<float>();
				const auto unitSphereArea = 2.f*unitHemisphereArea;

				float lightFlux = unitHemisphereArea*getFactorLuminosity();
				auto sideArea = [&](const irr::core::vectorSIMDf& a, const irr::core::vectorSIMDf& b, const irr::core::vectorSIMDf& c) -> float
				{
					irr::core::vectorSIMDf v[3];
					transform.transformVect(v[0], a);
					transform.transformVect(v[1], b);
					transform.transformVect(v[2], c);
					return irr::core::length(irr::core::cross(v[1] - v[0], v[2] - v[0])).x;
				};
				switch (type)
				{
					case ET_CONSTANT: // no-op because factor is in Watts / Wavelength / steradian
						break;
					case ET_CUBE:
						{
							float cubeArea = sideArea(irr::core::vectorSIMDf(-1.f,-1.f,0.f),irr::core::vectorSIMDf(1.f,-1.f,0.f),irr::core::vectorSIMDf(-1.f,1.f,0.f));
							cubeArea += sideArea(irr::core::vectorSIMDf(-1.f,0.f,-1.f),irr::core::vectorSIMDf(1.f,0.f,-1.f),irr::core::vectorSIMDf(-1.f,0.f,1.f));
							cubeArea += sideArea(irr::core::vectorSIMDf(0.f,-1.f,-1.f),irr::core::vectorSIMDf(0.f,1.f,-1.f),irr::core::vectorSIMDf(0.f,-1.f,1.f));
							lightFlux *= cubeArea*2.f;
							assert(false);
						}
						break;
					case ET_RECTANGLE:
						{
							irr::core::vectorSIMDf v[3];
							transform.transformVect(v[0],irr::core::vectorSIMDf(-1.f,-1.f,0.f));
							transform.transformVect(v[1],irr::core::vectorSIMDf( 1.f,-1.f,0.f));
							transform.transformVect(v[2],irr::core::vectorSIMDf(-1.f, 1.f,0.f));
							lightFlux *= irr::core::length(irr::core::cross(v[1]-v[0],v[2]-v[0])).x;
						}
						break;
					case ET_ELLIPSOID:
						_IRR_FALLTHROUGH;
					//! TODO: check if can analytically compute arbitrary 3x3 transform cylinder area 
					case ET_CYLINDER:
						_IRR_FALLTHROUGH;
					//! TODO: check if can analytically compute arbitrary unit disk transformed by 3x3 matrix area
					case ET_DISK:
						_IRR_FALLTHROUGH;
					case ET_TRIANGLE:
						lightFlux *= triangulizationArea;
						break;
					default:
						assert(false);
						break;
				}
				return lightFlux;
			}

			//! different lights use different measures of their strength (this already has the reciprocal of the light PDF factored in)
			alignas(16) float strengthFactor[3];
			//! type is second member due to alignment issues
			E_TYPE type;
			//! useful for analytical shapes
			union
			{
				irr::core::matrix3x4SIMD transform;
				struct Triangle
				{
					irr::core::vectorSIMDf vertices[3];
				} triangle;
			};
			/*
			union
			{
				AreaSphere sphere;
			};
			*/
		};
		static_assert(sizeof(SLight)==64u,"Can't keep alignment straight!");

		Renderer(irr::video::IVideoDriver* _driver, irr::asset::IAssetManager* _assetManager, irr::scene::ISceneManager* _smgr);

		void init(const irr::asset::SAssetBundle& meshes, uint32_t rayBufferSize=512u*1024u*1024u);

		void deinit();

		void render();

		auto* getColorBuffer() { return m_colorBuffer; }

		const auto& getSceneBound() const { return sceneBound; }

		auto getTotalSamplesComputed() const { return m_totalSamplesComputed; }
    protected:
        ~Renderer();

        irr::video::IVideoDriver* m_driver;
		irr::video::E_MATERIAL_TYPE nonInstanced;
		uint32_t m_raygenProgram, m_compostProgram;
		irr::asset::IAssetManager* m_assetManager;
		irr::scene::ISceneManager* m_smgr;
		irr::core::smart_refctd_ptr<irr::ext::RadeonRays::Manager> m_rrManager;

		irr::core::smart_refctd_ptr<irr::video::ITexture> m_depth,m_albedo,m_normals,m_accumulation,m_tonemapOutput;
		irr::video::IFrameBuffer* m_colorBuffer,* m_gbuffer,* tmpTonemapBuffer;

		uint32_t m_workGroupCount[2];
		uint32_t m_samplesPerDispatch;
		uint32_t m_totalSamplesComputed;
		uint32_t m_rayCount;
		uint32_t m_framesDone;
		irr::core::smart_refctd_ptr<irr::video::IGPUBuffer> m_rayBuffer;
		irr::core::smart_refctd_ptr<irr::video::IGPUBuffer> m_intersectionBuffer;
		irr::core::smart_refctd_ptr<irr::video::IGPUBuffer> m_rayCountBuffer;
		std::pair<::RadeonRays::Buffer*,cl_mem> m_rayBufferAsRR;
		std::pair<::RadeonRays::Buffer*,cl_mem> m_intersectionBufferAsRR;
		std::pair<::RadeonRays::Buffer*,cl_mem> m_rayCountBufferAsRR;

		irr::core::vector<irr::core::smart_refctd_ptr<irr::scene::IMeshSceneNode> > nodes;
		irr::core::aabbox3df sceneBound;
		irr::ext::RadeonRays::Manager::MeshBufferRRShapeCache rrShapeCache;
		irr::ext::RadeonRays::Manager::MeshNodeRRInstanceCache rrInstances;

		irr::core::smart_refctd_ptr<irr::video::IGPUBuffer> m_lightCDFBuffer;
		irr::core::smart_refctd_ptr<irr::video::IGPUBuffer> m_lightBuffer;
};

#endif