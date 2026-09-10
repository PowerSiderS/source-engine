//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//===========================================================================//

#if defined( ANDROID )
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>
#define __gl_h_ 1
#define NO_SDL_GLEXT 1
#elif defined( _WIN32 )
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include "utlvector.h"
#include "materialsystem/imaterialsystem.h"
#include "IHardwareConfigInternal.h"
#include "shadersystem.h"
#include "shaderapi/ishaderutil.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imesh.h"
#include "tier0/dbg.h"
#include "materialsystem/idebugtextureinfo.h"
#include "materialsystem/deformations.h"
#include "mathlib/vmatrix.h"
#include "shaderapidx9/meshbase.h"
#include "appframework/ilaunchermgr.h"
#include "bitmap/imageformat.h"
#include "pixelwriter.h"
#include "imaterialinternal.h"
#include "itextureinternal.h"

#if defined( USE_SDL )
#include "SDL.h"
#include "SDL_video.h"
#endif

static VertexFormat_t ComputeGLVertexFormat( unsigned int flags, int nTexCoordArraySize, int* pTexCoordDimensions, int numBoneWeights, int userDataSize )
{
	VertexFormat_t fmt = flags & ~VERTEX_FORMAT_USE_EXACT_FORMAT;
	fmt &= ~VERTEX_FORMAT_COMPRESSED;

	if ( numBoneWeights > 0 )
	{
		fmt |= VERTEX_BONEWEIGHT( 2 );
	}

	fmt |= VERTEX_USERDATA_SIZE( userDataSize );

	int maxTexCoords = ( nTexCoordArraySize < (int)VERTEX_MAX_TEXTURE_COORDINATES ) ? nTexCoordArraySize : (int)VERTEX_MAX_TEXTURE_COORDINATES;
	for ( int i = 0; i < maxTexCoords; ++i )
	{
		if ( pTexCoordDimensions )
		{
			fmt |= VERTEX_TEXCOORD_SIZE( (TextureStage_t)i, pTexCoordDimensions[i] );
		}
		else
		{
			fmt |= VERTEX_TEXCOORD_SIZE( (TextureStage_t)i, 2 );
		}
	}
	return fmt;
}

//-----------------------------------------------------------------------------
// The empty mesh
//-----------------------------------------------------------------------------
class CMeshGL : public IMesh
{
public:
	CMeshGL( bool bIsDynamic );
	virtual ~CMeshGL();

	// FIXME: Make this work! Unsupported methods of IIndexBuffer + IVertexBuffer
	virtual bool Lock( int nMaxIndexCount, bool bAppend, IndexDesc_t& desc );
	virtual void Unlock( int nWrittenIndexCount, IndexDesc_t& desc );
	virtual void ModifyBegin( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t& desc );
	virtual void ModifyEnd( IndexDesc_t& desc );
	virtual void Spew( int nIndexCount, const IndexDesc_t & desc );
	virtual void ValidateData( int nIndexCount, const IndexDesc_t &desc );
	virtual bool Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc );
	virtual void Unlock( int nVertexCount, VertexDesc_t &desc );
	virtual void Spew( int nVertexCount, const VertexDesc_t &desc );
	virtual void ValidateData( int nVertexCount, const VertexDesc_t & desc );
	virtual bool IsDynamic() const { return m_bIsDynamic; }
	virtual void BeginCastBuffer( VertexFormat_t format ) {}
	virtual void BeginCastBuffer( MaterialIndexFormat_t format ) {}
	virtual void EndCastBuffer( ) {}
	virtual int GetRoomRemaining() const { return 0; }
	virtual MaterialIndexFormat_t IndexFormat() const { return MATERIAL_INDEX_FORMAT_16BIT; }

	void LockMesh( int numVerts, int numIndices, MeshDesc_t& desc );
	void UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc );

	void ModifyBeginEx( bool bReadOnly, int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc );
	void ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc );
	void ModifyEnd( MeshDesc_t& desc );

	// returns the # of vertices (static meshes only)
	int  VertexCount() const;

	// Sets the primitive type
	void SetPrimitiveType( MaterialPrimitiveType_t type );

	// Draws the entire mesh
	void Draw(int firstIndex, int numIndices);

	void Draw(CPrimList *pPrims, int nPrims);

	// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
	virtual void CopyToMeshBuilder(
		int iStartVert,		// Which vertices to copy.
		int nVerts,
		int iStartIndex,	// Which indices to copy.
		int nIndices,
		int indexOffset,	// This is added to each index.
		CMeshBuilder &builder );

	// Spews the mesh data
	void Spew( int numVerts, int numIndices, const MeshDesc_t & desc );

	void ValidateData( int numVerts, int numIndices, const MeshDesc_t & desc );

	// gets the associated material
	IMaterial* GetMaterial();
	virtual void SetMaterial( IMaterial* pMaterial ) { m_pMaterial = pMaterial; }

	void SetColorMesh( IMesh *pColorMesh, int nVertexOffset )
	{
	}


	virtual int IndexCount() const
	{
		return m_nIndexCount;
	}

	virtual void SetFlexMesh( IMesh *pMesh, int nVertexOffset ) {}

	virtual void DisableFlexMesh() {}

	virtual void MarkAsDrawn() {}

	virtual unsigned ComputeMemoryUsed() { return m_nVertexAllocSize + m_nIndexAllocSize; }

	virtual void SetVertexFormat( VertexFormat_t format ) { m_VertexFormat = format; }
	virtual VertexFormat_t GetVertexFormat() const { return m_VertexFormat; }

	virtual IMesh *GetMesh()
	{
		return this;
	}

public:
	void EnsureVertexAllocation( int nBytes );
	void EnsureIndexAllocation( int nBytes );

	unsigned char* m_pVertexMemory;
	int m_nVertexAllocSize;
	unsigned char* m_pIndexMemory;
	int m_nIndexAllocSize;
	bool m_bIsDynamic;
	MaterialPrimitiveType_t m_Type;
	VertexFormat_t m_VertexFormat;
	IMaterial* m_pMaterial;
	int m_nIndexCount;
	int m_nVertexCount;
	GLuint m_hVBO;
	GLuint m_hIBO;
	bool m_bVBOValid;
	bool m_bIBOValid;
	CMeshGL* m_pVertexOverride;
	CMeshGL* m_pIndexOverride;
	void SetVertexOverride( CMeshGL* pMesh ) { m_pVertexOverride = pMesh; }
	void SetIndexOverride( CMeshGL* pMesh ) { m_pIndexOverride = pMesh; }
};


//-----------------------------------------------------------------------------
// The empty shader shadow
//-----------------------------------------------------------------------------
class CShaderShadowGL : public IShaderShadow
{
public:
	CShaderShadowGL();
	virtual ~CShaderShadowGL();

	// Sets the default *shadow* state
	void SetDefaultState();

	// Methods related to depth buffering
	void DepthFunc( ShaderDepthFunc_t depthFunc );
	void EnableDepthWrites( bool bEnable );
	void EnableDepthTest( bool bEnable );
	void EnablePolyOffset( PolygonOffsetMode_t nOffsetMode );

	// Suppresses/activates color writing
	void EnableColorWrites( bool bEnable );
	void EnableAlphaWrites( bool bEnable );

	// Methods related to alpha blending
	void EnableBlending( bool bEnable );
	void BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor );

	// Alpha testing
	void EnableAlphaTest( bool bEnable );
	void AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ );

	// Wireframe/filled polygons
	void PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode );

	// Back face culling
	void EnableCulling( bool bEnable );

	// constant color + transparency
	void EnableConstantColor( bool bEnable );

	// Indicates the vertex format for use with a vertex shader
	// The flags to pass in here come from the VertexFormatFlags_t enum
	// If pTexCoordDimensions is *not* specified, we assume all coordinates
	// are 2-dimensional
	void VertexShaderVertexFormat( unsigned int nFlags,
		int nTexCoordCount, int* pTexCoordDimensions, int nUserDataSize );

	// Indicates we're going to light the model
	void EnableLighting( bool bEnable );
	void EnableSpecular( bool bEnable );

	// vertex blending
	void EnableVertexBlend( bool bEnable );

	// per texture unit stuff
	void OverbrightValue( TextureStage_t stage, float value );
	void EnableTexture( Sampler_t stage, bool bEnable );
	void EnableTexGen( TextureStage_t stage, bool bEnable );
	void TexGen( TextureStage_t stage, ShaderTexGenParam_t param );

	// alternate method of specifying per-texture unit stuff, more flexible and more complicated
	// Can be used to specify different operation per channel (alpha/color)...
	void EnableCustomPixelPipe( bool bEnable );
	void CustomTextureStages( int stageCount );
	void CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel,
		ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 );

	// indicates what per-vertex data we're providing
	void DrawFlags( unsigned int drawFlags );

	// A simpler method of dealing with alpha modulation
	void EnableAlphaPipe( bool bEnable );
	void EnableConstantAlpha( bool bEnable );
	void EnableVertexAlpha( bool bEnable );
	void EnableTextureAlpha( TextureStage_t stage, bool bEnable );

	// GR - Separate alpha blending
	void EnableBlendingSeparateAlpha( bool bEnable );
	void BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor );

	// Sets the vertex and pixel shaders
	void SetVertexShader( const char *pFileName, int vshIndex );
	void SetPixelShader( const char *pFileName, int pshIndex );

	// Convert from linear to gamma color space on writes to frame buffer.
	void EnableSRGBWrite( bool bEnable )
	{
	}

	void EnableSRGBRead( Sampler_t stage, bool bEnable )
	{
	}

	virtual void FogMode( ShaderFogMode_t fogMode )
	{
	}

	virtual void DisableFogGammaCorrection( bool bDisable )
	{
	}

	virtual void SetDiffuseMaterialSource( ShaderMaterialSource_t materialSource )
	{
	}

	virtual void SetMorphFormat( MorphFormat_t flags )
	{
	}

	virtual void EnableStencil( bool bEnable )
	{
	}
	virtual void StencilFunc( ShaderStencilFunc_t stencilFunc )
	{
	}
	virtual void StencilPassOp( ShaderStencilOp_t stencilOp )
	{
	}
	virtual void StencilFailOp( ShaderStencilOp_t stencilOp )
	{
	}
	virtual void StencilDepthFailOp( ShaderStencilOp_t stencilOp )
	{
	}
	virtual void StencilReference( int nReference )
	{
	}
	virtual void StencilMask( int nMask )
	{
	}
	virtual void StencilWriteMask( int nMask )
	{
	}

	virtual void ExecuteCommandBuffer( uint8 *pBuf )
	{
	}
	// Alpha to coverage
	void EnableAlphaToCoverage( bool bEnable );

	virtual void SetShadowDepthFiltering( Sampler_t stage )
	{
	}

	virtual void BlendOp( ShaderBlendOp_t blendOp ) {}
	virtual void BlendOpSeparateAlpha( ShaderBlendOp_t blendOp ) {}

	bool m_IsTranslucent;
	bool m_IsAlphaTested;
	bool m_bIsDepthWriteEnabled;
	bool m_bDepthTestEnabled;
	ShaderBlendFactor_t m_SrcBlend, m_DstBlend;
	bool m_bColorWriteEnabled;
	bool m_bAlphaWriteEnabled;
	bool m_bUsesVertexAndPixelShaders;
	VertexFormat_t m_VertexUsage;
};


//-----------------------------------------------------------------------------
// The DX8 implementation of the shader device
//-----------------------------------------------------------------------------
class CShaderDeviceGL : public IShaderDevice
{
public:
	CShaderDeviceGL() : m_DynamicMesh( true ), m_Mesh( true ) {}

	// Methods of IShaderDevice
	virtual int GetCurrentAdapter() const { return 0; }
	virtual bool IsUsingGraphics() const { return true; }
	virtual void SpewDriverInfo() const;
	virtual ImageFormat GetBackBufferFormat() const { return IMAGE_FORMAT_RGB888; }
	virtual void GetBackBufferDimensions( int& width, int& height ) const;
	virtual int  StencilBufferBits() const { return 0; }
	virtual bool IsAAEnabled() const { return false; }
	virtual void Present( )
	{
#if defined( USE_SDL )
		SDL_Window *pWin = SDL_GL_GetCurrentWindow();
		if ( pWin && SDL_GL_GetCurrentContext() )
		{
			SDL_ClearError();
			SDL_GL_SwapWindow( pWin );
			const char *pError = SDL_GetError();
			static int s_nSwapErrors = 0;
			if ( pError && pError[0] && s_nSwapErrors++ < 8 )
				Warning( "[GL] SDL swap failed: %s\n", pError );
		}
		else
#endif
		{
#if defined( ANDROID )
			EGLDisplay display = eglGetCurrentDisplay();
			EGLSurface surface = eglGetCurrentSurface( EGL_DRAW );
			if ( display != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE )
			{
				if ( !eglSwapBuffers( display, surface ) )
				{
					static int s_nSwapErrors = 0;
					if ( s_nSwapErrors++ < 8 )
						Warning( "[GL] EGL swap failed: 0x%x\n", eglGetError() );
				}
			}
			else
			{
				static int s_nMissingContext = 0;
				if ( s_nMissingContext++ < 8 )
					Warning( "[GL] Present has no current window surface/context\n" );
				return;
			}
#endif
		}

		static int s_nPresentCount = 0;
		if ( ++s_nPresentCount <= 5 || ( s_nPresentCount % 300 ) == 0 )
		{
			int w = 0, h = 0;
			GetBackBufferDimensions( w, h );
			Msg( "[GL] Present frame %d (backbuffer %dx%d)\n", s_nPresentCount, w, h );
		}
	}
	virtual void GetWindowSize( int &width, int &height ) const;
	virtual bool AddView( void* hwnd );
	virtual void RemoveView( void* hwnd );
	virtual void SetView( void* hwnd );
	virtual void ReleaseResources();
	virtual void ReacquireResources();
	virtual IMesh* CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial * pMaterial = NULL );
	virtual void DestroyStaticMesh( IMesh* mesh );
	virtual IShaderBuffer* CompileShader( const char *pProgram, size_t nBufLen, const char *pShaderVersion ) { return NULL; }
	virtual VertexShaderHandle_t CreateVertexShader( IShaderBuffer* pShaderBuffer ) { return VERTEX_SHADER_HANDLE_INVALID; }
	virtual void DestroyVertexShader( VertexShaderHandle_t hShader ) {}
	virtual GeometryShaderHandle_t CreateGeometryShader( IShaderBuffer* pShaderBuffer ) { return GEOMETRY_SHADER_HANDLE_INVALID; }
	virtual void DestroyGeometryShader( GeometryShaderHandle_t hShader ) {}
	virtual PixelShaderHandle_t CreatePixelShader( IShaderBuffer* pShaderBuffer ) { return PIXEL_SHADER_HANDLE_INVALID; }
	virtual void DestroyPixelShader( PixelShaderHandle_t hShader ) {}
	virtual IVertexBuffer *CreateVertexBuffer( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char *pBudgetGroup );
	virtual void DestroyVertexBuffer( IVertexBuffer *pVertexBuffer );
	virtual IIndexBuffer *CreateIndexBuffer( ShaderBufferType_t bufferType, MaterialIndexFormat_t fmt, int nIndexCount, const char *pBudgetGroup );
	virtual void DestroyIndexBuffer( IIndexBuffer *pIndexBuffer );
	virtual IVertexBuffer *GetDynamicVertexBuffer( int streamID, VertexFormat_t vertexFormat, bool bBuffered );
	virtual IIndexBuffer *GetDynamicIndexBuffer( MaterialIndexFormat_t fmt, bool bBuffered );
	virtual void SetHardwareGammaRamp( float fGamma, float fGammaTVRangeMin, float fGammaTVRangeMax, float fGammaTVExponent, bool bTVEnabled ) {}
	virtual void EnableNonInteractiveMode( MaterialNonInteractiveMode_t mode, ShaderNonInteractiveInfo_t *pInfo ) {}
	virtual void RefreshFrontBufferNonInteractive( ) {}
	virtual void HandleThreadEvent( uint32 threadEvent ) {}

#ifdef DX_TO_GL_ABSTRACTION
	virtual void DoStartupShaderPreloading( void ) {}
#endif

	virtual char *GetDisplayDeviceName() OVERRIDE { return ""; }

private:
	CMeshGL m_Mesh;
	CMeshGL m_DynamicMesh;
};

static CShaderDeviceGL g_ShaderDeviceGL;

// FIXME: Remove; it's for backward compat with the materialsystem only for now
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderDeviceGL, IShaderDevice,
								  SHADER_DEVICE_INTERFACE_VERSION, g_ShaderDeviceGL )


//-----------------------------------------------------------------------------
// The DX8 implementation of the shader device
//-----------------------------------------------------------------------------
class CShaderDeviceMgrGL : public IShaderDeviceMgr
{
public:
	// Methods of IAppSystem
	virtual bool Connect( CreateInterfaceFn factory );
	virtual void Disconnect();
	virtual void *QueryInterface( const char *pInterfaceName );
	virtual InitReturnVal_t Init();
	virtual void Shutdown();

public:
	// Methods of IShaderDeviceMgr
	virtual int	 GetAdapterCount() const;
	virtual void GetAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const;
	virtual bool GetRecommendedConfigurationInfo( int nAdapter, int nDXLevel, KeyValues *pKeyValues );
	virtual int	 GetModeCount( int adapter ) const;
	virtual void GetModeInfo( ShaderDisplayMode_t *pInfo, int nAdapter, int mode ) const;
	virtual void GetCurrentModeInfo( ShaderDisplayMode_t* pInfo, int nAdapter ) const;
	virtual bool SetAdapter( int nAdapter, int nFlags );
	virtual CreateInterfaceFn SetMode( void *hWnd, int nAdapter, const ShaderDeviceInfo_t& mode );
	virtual void AddModeChangeCallback( ShaderModeChangeCallbackFunc_t func ) {}
	virtual void RemoveModeChangeCallback( ShaderModeChangeCallbackFunc_t func ) {}
};

static CShaderDeviceMgrGL g_ShaderDeviceMgrGL;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderDeviceMgrGL, IShaderDeviceMgr,
								  SHADER_DEVICE_MGR_INTERFACE_VERSION, g_ShaderDeviceMgrGL )


class CMatrixStack
{
public:
	CMatrixStack()
	{
		m_Stack.AddToTail();
		m_Stack[0].Identity();
	}

	void Push()
	{
		VMatrix top = Top();
		m_Stack.AddToTail( top );
	}

	void Pop()
	{
		if ( m_Stack.Count() > 1 )
		{
			m_Stack.Remove( m_Stack.Count() - 1 );
		}
	}

	void LoadIdentity()
	{
		Top().Identity();
	}

	void LoadMatrix( const float *m )
	{
		if ( m )
		{
			memcpy( Top().Base(), m, 16 * sizeof(float) );
		}
	}

	void MultMatrix( const float *m )
	{
		if ( m )
		{
			VMatrix inMat;
			memcpy( inMat.Base(), m, 16 * sizeof(float) );
			VMatrix result;
			MatrixMultiply( Top(), inMat, result );
			Top() = result;
		}
	}

	void MultMatrixLocal( const float *m )
	{
		if ( m )
		{
			VMatrix inMat;
			memcpy( inMat.Base(), m, 16 * sizeof(float) );
			VMatrix result;
			MatrixMultiply( inMat, Top(), result );
			Top() = result;
		}
	}

	void MultMatrixLocal( const VMatrix &m )
	{
		VMatrix result;
		MatrixMultiply( m, Top(), result );
		Top() = result;
	}

	void ScaleLocal( float x, float y, float z )
	{
		VMatrix scaleMat;
		MatrixBuildScale( scaleMat, x, y, z );
		MultMatrixLocal( scaleMat );
	}

	void TranslateLocal( float x, float y, float z )
	{
		VMatrix transMat;
		MatrixBuildTranslation( transMat, x, y, z );
		MultMatrixLocal( transMat );
	}

	void RotateAxisLocal( const Vector &axis, float angleDegrees )
	{
		VMatrix rotMat;
		MatrixBuildRotationAboutAxis( rotMat, axis, angleDegrees );
		MultMatrixLocal( rotMat );
	}

	const VMatrix& Top() const
	{
		return m_Stack[m_Stack.Count() - 1];
	}

	VMatrix& Top()
	{
		return m_Stack[m_Stack.Count() - 1];
	}

private:
	CUtlVector<VMatrix> m_Stack;
};

static GLuint GetSolidColorTexture( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	GLuint tex = 0;
	glGenTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, tex );
	unsigned char pixels[4] = { r, g, b, a };
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	return tex;
}

static GLuint GetWhiteTexture()
{
	static GLuint s_hWhite = 0;
	if ( !s_hWhite )
	{
		s_hWhite = GetSolidColorTexture( 255, 255, 255, 255 );
	}
	return s_hWhite;
}

static GLuint GetBlackTexture()
{
	static GLuint s_hBlack = 0;
	if ( !s_hBlack )
	{
		s_hBlack = GetSolidColorTexture( 0, 0, 0, 255 );
	}
	return s_hBlack;
}

static GLuint GetGreyTexture()
{
	static GLuint s_hGrey = 0;
	if ( !s_hGrey )
	{
		s_hGrey = GetSolidColorTexture( 128, 128, 128, 255 );
	}
	return s_hGrey;
}

//-----------------------------------------------------------------------------
// The DX8 implementation of the shader API
//-----------------------------------------------------------------------------
class CShaderAPIGL : public IShaderAPI, public IHardwareConfigInternal, public IDebugTextureInfo
{
public:
	// constructor, destructor
	CShaderAPIGL( );
	virtual ~CShaderAPIGL();

	// IDebugTextureInfo implementation.
public:

	virtual bool IsDebugTextureListFresh( int numFramesAllowed = 1 ) { return false; }
	virtual bool SetDebugTextureRendering( bool bEnable ) { return false; }
	virtual void EnableDebugTextureList( bool bEnable ) {}
	virtual void EnableGetAllTextures( bool bEnable ) {}
	virtual KeyValues* GetDebugTextureList() { return NULL; }
	virtual int GetTextureMemoryUsed( TextureMemoryType eTextureMemory ) { return 0; }

	// Methods of IShaderDynamicAPI
	virtual void GetBackBufferDimensions( int& width, int& height ) const
	{
		g_ShaderDeviceGL.GetBackBufferDimensions( width, height );
	}
	virtual void GetCurrentColorCorrection( ShaderColorCorrectionInfo_t* pInfo )
	{
		pInfo->m_bIsEnabled = false;
		pInfo->m_nLookupCount = 0;
		pInfo->m_flDefaultWeight = 0.0f;
	}


	// Methods of IShaderAPI
public:
	virtual void SetViewports( int nCount, const ShaderViewport_t* pViewports );
	virtual int GetViewports( ShaderViewport_t* pViewports, int nMax ) const;
	virtual void ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil, int renderTargetWidth, int renderTargetHeight );
	virtual void ClearColor3ub( unsigned char r, unsigned char g, unsigned char b );
	virtual void ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	virtual void BindVertexShader( VertexShaderHandle_t hVertexShader ) {}
	virtual void BindGeometryShader( GeometryShaderHandle_t hGeometryShader ) {}
	virtual void BindPixelShader( PixelShaderHandle_t hPixelShader ) {}
	virtual void SetRasterState( const ShaderRasterState_t& state ) {}
	virtual void MarkUnusedVertexFields( unsigned int nFlags, int nTexCoordCount, bool *pUnusedTexCoords ) {}
	virtual bool OwnGPUResources( bool bEnable ) { return false; }

	virtual bool DoRenderTargetsNeedSeparateDepthBuffer() const;

	// Used to clear the transition table when we know it's become invalid.
	void ClearSnapshots();

	// Sets the mode...
	bool SetMode( void* hwnd, int nAdapter, const ShaderDeviceInfo_t &info )
	{
		return true;
	}

	void ChangeVideoMode( const ShaderDeviceInfo_t &info )
	{
	}

	// Called when the dx support level has changed
	virtual void DXSupportLevelChanged() {}

	virtual void EnableUserClipTransformOverride( bool bEnable ) {}
	virtual void UserClipTransform( const VMatrix &worldToView ) {}

	// Sets the default *dynamic* state
	void SetDefaultState( );

	// Returns the snapshot id for the shader state
	StateSnapshot_t	 TakeSnapshot( );

	// Returns true if the state snapshot is transparent
	bool IsTranslucent( StateSnapshot_t id ) const;
	bool IsAlphaTested( StateSnapshot_t id ) const;
	bool UsesVertexAndPixelShaders( StateSnapshot_t id ) const;
	virtual bool IsDepthWriteEnabled( StateSnapshot_t id ) const;

	// Gets the vertex format for a set of snapshot ids
	VertexFormat_t ComputeVertexFormat( int numSnapshots, StateSnapshot_t* pIds ) const;

	// Gets the vertex format for a set of snapshot ids
	VertexFormat_t ComputeVertexUsage( int numSnapshots, StateSnapshot_t* pIds ) const;

	// Begins a rendering pass that uses a state snapshot
	void BeginPass( StateSnapshot_t snapshot  );

	// Uses a state snapshot
	void UseSnapshot( StateSnapshot_t snapshot );

	// Use this to get the mesh builder that allows us to modify vertex data
	CMeshBuilder* GetVertexModifyBuilder();

	// Sets the color to modulate by
	void Color3f( float r, float g, float b );
	void Color3fv( float const* pColor );
	void Color4f( float r, float g, float b, float a );
	void Color4fv( float const* pColor );

	// Faster versions of color
	void Color3ub( unsigned char r, unsigned char g, unsigned char b );
	void Color3ubv( unsigned char const* rgb );
	void Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Color4ubv( unsigned char const* rgba );

	// Sets the lights
	void SetLight( int lightNum, const LightDesc_t& desc );
	void SetLightingOrigin( Vector vLightingOrigin );
	void SetAmbientLight( float r, float g, float b );
	void SetAmbientLightCube( Vector4D cube[6] );

	// Get the lights
	int GetMaxLights( void ) const;
	const LightDesc_t& GetLight( int lightNum ) const;

	// Render state for the ambient light cube (vertex shaders)
	void SetVertexShaderStateAmbientLightCube();
	void SetPixelShaderStateAmbientLightCube( int pshReg, bool bForceToBlack = false )
	{
	}

	float GetAmbientLightCubeLuminance(void)
	{
		return 0.0f;
	}

	void SetSkinningMatrices();

	// Lightmap texture binding
	void BindLightmap( TextureStage_t stage );
	void BindLightmapAlpha( TextureStage_t stage )
	{
	}
	void BindBumpLightmap( TextureStage_t stage );
	void BindFullbrightLightmap( TextureStage_t stage );
	void BindWhite( TextureStage_t stage );
	void BindBlack( TextureStage_t stage );
	void BindGrey( TextureStage_t stage );
	void BindFBTexture( TextureStage_t stage, int textureIdex );
	void CopyRenderTargetToTexture( ShaderAPITextureHandle_t texID );

	void CopyRenderTargetToTextureEx( ShaderAPITextureHandle_t texID, int nRenderTargetID, Rect_t *pSrcRect, Rect_t *pDstRect );

	void CopyTextureToRenderTargetEx( int nRenderTargetID, ShaderAPITextureHandle_t textureHandle, Rect_t *pSrcRect, Rect_t *pDstRect )
	{
	}

	// Special system flat normal map binding.
	void BindFlatNormalMap( TextureStage_t stage );
	void BindNormalizationCubeMap( TextureStage_t stage );
	void BindSignedNormalizationCubeMap( TextureStage_t stage );

	// Set the number of bone weights
	void SetNumBoneWeights( int numBones );
	void EnableHWMorphing( bool bEnable );

	// Flushes any primitives that are buffered
	void FlushBufferedPrimitives();

	// Gets the dynamic mesh; note that you've got to render the mesh
	// before calling this function a second time. Clients should *not*
	// call DestroyStaticMesh on the mesh returned by this call.
	IMesh* GetDynamicMesh( IMaterial* pMaterial, int nHWSkinBoneCount, bool buffered, IMesh* pVertexOverride, IMesh* pIndexOverride );
	IMesh* GetDynamicMeshEx( IMaterial* pMaterial, VertexFormat_t fmt, int nHWSkinBoneCount, bool buffered, IMesh* pVertexOverride, IMesh* pIndexOverride );

	IMesh* GetFlexMesh();

	// Renders a single pass of a material
	void RenderPass( int nPass, int nPassCount );

	// stuff related to matrix stacks
	void MatrixMode( MaterialMatrixMode_t matrixMode );
	void PushMatrix();
	void PopMatrix();
	void LoadMatrix( float *m );
	void LoadBoneMatrix( int boneIndex, const float *m ) {}
	void MultMatrix( float *m );
	void MultMatrixLocal( float *m );
	void GetMatrix( MaterialMatrixMode_t matrixMode, float *dst );
	void LoadIdentity( void );
	void LoadCameraToWorld( void );
	void Ortho( double left, double top, double right, double bottom, double zNear, double zFar );
	void PerspectiveX( double fovx, double aspect, double zNear, double zFar );
	void PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right );
	void PickMatrix( int x, int y, int width, int height );
	void Rotate( float angle, float x, float y, float z );
	void Translate( float x, float y, float z );
	void Scale( float x, float y, float z );
	void ScaleXY( float x, float y );

	// Fog methods...
	void FogMode( MaterialFogMode_t fogMode );
	void FogStart( float fStart );
	void FogEnd( float fEnd );
	void SetFogZ( float fogZ );
	void FogMaxDensity( float flMaxDensity );
	void GetFogDistances( float *fStart, float *fEnd, float *fFogZ );
	void FogColor3f( float r, float g, float b );
	void FogColor3fv( float const* rgb );
	void FogColor3ub( unsigned char r, unsigned char g, unsigned char b );
	void FogColor3ubv( unsigned char const* rgb );

	virtual void SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b );
	virtual void SceneFogMode( MaterialFogMode_t fogMode );
	virtual void GetSceneFogColor( unsigned char *rgb );
	virtual MaterialFogMode_t GetSceneFogMode( );
	virtual int GetPixelFogCombo( );

	void SetHeightClipZ( float z );
	void SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode );

	void SetClipPlane( int index, const float *pPlane );
	void EnableClipPlane( int index, bool bEnable );

	void SetFastClipPlane( const float *pPlane );
	void EnableFastClip( bool bEnable );

	// We use smaller dynamic VBs during level transitions, to free up memory
	virtual int  GetCurrentDynamicVBSize( void );
	virtual void DestroyVertexBuffers( bool bExitingLevel = false );

	// Sets the vertex and pixel shaders
	void SetVertexShaderIndex( int vshIndex );
	void SetPixelShaderIndex( int pshIndex );

	// Sets the constant register for vertex and pixel shaders
	void SetVertexShaderConstant( int var, float const* pVec, int numConst = 1, bool bForce = false );
	void SetBooleanVertexShaderConstant( int var, BOOL const* pVec, int numConst = 1, bool bForce = false );
	void SetIntegerVertexShaderConstant( int var, int const* pVec, int numConst = 1, bool bForce = false );
	void SetPixelShaderConstant( int var, float const* pVec, int numConst = 1, bool bForce = false );
	void SetBooleanPixelShaderConstant( int var, BOOL const* pVec, int numBools = 1, bool bForce = false );
	void SetIntegerPixelShaderConstant( int var, int const* pVec, int numIntVecs = 1, bool bForce = false );

	void InvalidateDelayedShaderConstants( void );

	// Gamma<->Linear conversions according to the video hardware we're running on
	float GammaToLinear_HardwareSpecific( float fGamma ) const;
	float LinearToGamma_HardwareSpecific( float fLinear ) const;

	//Set's the linear->gamma conversion textures to use for this hardware for both srgb writes enabled and disabled(identity)
	void SetLinearToGammaConversionTextures( ShaderAPITextureHandle_t hSRGBWriteEnabledTexture, ShaderAPITextureHandle_t hIdentityTexture );

	// Cull mode
	void CullMode( MaterialCullMode_t cullMode );

	// Force writes only when z matches. . . useful for stenciling things out
	// by rendering the desired Z values ahead of time.
	void ForceDepthFuncEquals( bool bEnable );

	// Forces Z buffering on or off
	void OverrideDepthEnable( bool bEnable, bool bDepthEnable );
	void OverrideAlphaWriteEnable( bool bOverrideEnable, bool bAlphaWriteEnable );
	void OverrideColorWriteEnable( bool bOverrideEnable, bool bColorWriteEnable );

	// Sets the shade mode
	void ShadeMode( ShaderShadeMode_t mode );

	// Binds a particular material to render with
	void Bind( IMaterial* pMaterial );

	// Returns the nearest supported format
	ImageFormat GetNearestSupportedFormat( ImageFormat fmt, bool bFilteringRequired = true ) const;
	ImageFormat GetNearestRenderTargetFormat( ImageFormat fmt ) const;

	// Sets the texture state
	void BindTexture( Sampler_t stage, ShaderAPITextureHandle_t textureHandle );

	void SetRenderTarget( ShaderAPITextureHandle_t colorTextureHandle, ShaderAPITextureHandle_t depthTextureHandle );

	void SetRenderTargetEx( int nRenderTargetID, ShaderAPITextureHandle_t colorTextureHandle, ShaderAPITextureHandle_t depthTextureHandle );

	// Indicates we're going to be modifying this texture
	// TexImage2D, TexSubImage2D, TexWrap, TexMinFilter, and TexMagFilter
	// all use the texture specified by this function.
	void ModifyTexture( ShaderAPITextureHandle_t textureHandle );

	// Texture management methods
	void TexImage2D( int level, int cubeFace, ImageFormat dstFormat, int zOffset, int width, int height,
							 ImageFormat srcFormat, bool bSrcIsTiled, void *imageData );
	void TexSubImage2D( int level, int cubeFace, int xOffset, int yOffset, int zOffset, int width, int height,
							 ImageFormat srcFormat, int srcStride, bool bSrcIsTiled, void *imageData );

	void TexImageFromVTF( IVTFTexture *pVTF, int iVTFFrame );

	bool TexLock( int level, int cubeFaceID, int xOffset, int yOffset,
									int width, int height, CPixelWriter& writer );
	void TexUnlock( );

	// These are bound to the texture, not the texture environment
	void TexMinFilter( ShaderTexFilterMode_t texFilterMode );
	void TexMagFilter( ShaderTexFilterMode_t texFilterMode );
	void TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode );
	void TexSetPriority( int priority );

	ShaderAPITextureHandle_t CreateTexture(
		int width,
		int height,
		int depth,
		ImageFormat dstImageFormat,
		int numMipLevels,
		int numCopies,
		int flags,
		const char *pDebugName,
		const char *pTextureGroupName );
	// Create a multi-frame texture (equivalent to calling "CreateTexture" multiple times, but more efficient)
	void CreateTextures(
		ShaderAPITextureHandle_t *pHandles,
		int count,
		int width,
		int height,
		int depth,
		ImageFormat dstImageFormat,
		int numMipLevels,
		int numCopies,
		int flags,
		const char *pDebugName,
		const char *pTextureGroupName );
	ShaderAPITextureHandle_t CreateDepthTexture( ImageFormat renderFormat, int width, int height, const char *pDebugName, bool bTexture );
	void DeleteTexture( ShaderAPITextureHandle_t textureHandle );
	bool IsTexture( ShaderAPITextureHandle_t textureHandle );
	bool IsTextureResident( ShaderAPITextureHandle_t textureHandle );

	// stuff that isn't to be used from within a shader
	void ClearBuffersObeyStencil( bool bClearColor, bool bClearDepth );
	void ClearBuffersObeyStencilEx( bool bClearColor, bool bClearAlpha, bool bClearDepth );
	void PerformFullScreenStencilOperation( void );
	void ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat );
	virtual void ReadPixels( Rect_t *pSrcRect, Rect_t *pDstRect, unsigned char *data, ImageFormat dstFormat, int nDstStride );

	// Selection mode methods
	int SelectionMode( bool selectionMode );
	void SelectionBuffer( unsigned int* pBuffer, int size );
	void ClearSelectionNames( );
	void LoadSelectionName( int name );
	void PushSelectionName( int name );
	void PopSelectionName();

	void FlushHardware();
	void ResetRenderState( bool bFullReset = true );

	void SetScissorRect( const int nLeft, const int nTop, const int nRight, const int nBottom, const bool bEnableScissor );

	// Can we download textures?
	virtual bool CanDownloadTextures() const;

	// Board-independent calls, here to unify how shaders set state
	// Implementations should chain back to IShaderUtil->BindTexture(), etc.

	// Use this to begin and end the frame
	void BeginFrame();
	void EndFrame();

	// returns current time
	double CurrentTime() const;

	// Get the current camera position in world space.
	void GetWorldSpaceCameraPosition( float * pPos ) const;

	// Members of IMaterialSystemHardwareConfig
	bool HasDestAlphaBuffer() const;
	bool HasStencilBuffer() const;
	virtual int  MaxViewports() const;
	virtual void OverrideStreamOffsetSupport( bool bOverrideEnabled, bool bEnableSupport ) {}
	virtual int  GetShadowFilterMode() const;
	int  StencilBufferBits() const;
	int	 GetFrameBufferColorDepth() const;
	int  GetSamplerCount() const;
	bool HasSetDeviceGammaRamp() const;
	bool SupportsCompressedTextures() const;
	VertexCompressionType_t SupportsCompressedVertices() const;
	bool SupportsVertexAndPixelShaders() const;
	bool SupportsPixelShaders_1_4() const;
	bool SupportsPixelShaders_2_0() const;
	bool SupportsPixelShaders_2_b() const;
	bool ActuallySupportsPixelShaders_2_b() const;
	bool SupportsStaticControlFlow() const;
	bool SupportsVertexShaders_2_0() const;
	bool SupportsShaderModel_3_0() const;
	int  MaximumAnisotropicLevel() const;
	int  MaxTextureWidth() const;
	int  MaxTextureHeight() const;
	int  MaxTextureAspectRatio() const;
	int  GetDXSupportLevel() const;
	const char *GetShaderDLLName() const
	{
		return "UNKNOWN";
	}
	int	 TextureMemorySize() const;
	bool SupportsOverbright() const;
	bool SupportsCubeMaps() const;
	bool SupportsMipmappedCubemaps() const;
	bool SupportsNonPow2Textures() const;
	int  GetTextureStageCount() const;
	int	 NumVertexShaderConstants() const;
	int	 NumBooleanVertexShaderConstants() const;
	int	 NumIntegerVertexShaderConstants() const;
	int	 NumPixelShaderConstants() const;
	int	 MaxNumLights() const;
	bool SupportsHardwareLighting() const;
	int	 MaxBlendMatrices() const;
	int	 MaxBlendMatrixIndices() const;
	int	 MaxVertexShaderBlendMatrices() const;
	int	 MaxUserClipPlanes() const;
	bool UseFastClipping() const
	{
		return false;
	}
	bool SpecifiesFogColorInLinearSpace() const;
	virtual bool SupportsSRGB() const;
	virtual bool FakeSRGBWrite() const;
	virtual bool CanDoSRGBReadFromRTs() const;
	virtual bool SupportsGLMixedSizeTargets() const;

	const char *GetHWSpecificShaderDLLName() const;
	bool NeedsAAClamp() const
	{
		return false;
	}
	bool SupportsSpheremapping() const;
	virtual int MaxHWMorphBatchCount() const { return 0; }

	// This is the max dx support level supported by the card
	virtual int	 GetMaxDXSupportLevel() const;

	bool ReadPixelsFromFrontBuffer() const;
	bool PreferDynamicTextures() const;
	virtual bool PreferReducedFillrate() const;
	bool HasProjectedBumpEnv() const;
	void ForceHardwareSync( void );

	int GetCurrentNumBones( void ) const;
	bool IsHWMorphingEnabled( void ) const;
	int GetCurrentLightCombo( void ) const;
	void GetDX9LightState( LightState_t *state ) const;
	MaterialFogMode_t GetCurrentFogType( void ) const;

	void RecordString( const char *pStr );

	void EvictManagedResources();

	void SetTextureTransformDimension( TextureStage_t textureStage, int dimension, bool projected );
	void DisableTextureTransform( TextureStage_t textureStage )
	{
	}
	void SetBumpEnvMatrix( TextureStage_t textureStage, float m00, float m01, float m10, float m11 );

	// Gets the lightmap dimensions
	virtual void GetLightmapDimensions( int *w, int *h );

	virtual void SyncToken( const char *pToken );

	// Setup standard vertex shader constants (that don't change)
	// This needs to be called anytime that overbright changes.
	virtual void SetStandardVertexShaderConstants( float fOverbright )
	{
	}

	// Level of anisotropic filtering
	virtual void SetAnisotropicLevel( int nAnisotropyLevel );

	bool SupportsHDR() const
	{
		return false;
	}
	HDRType_t GetHDRType() const
	{
		return HDR_TYPE_NONE;
	}
	HDRType_t GetHardwareHDRType() const
	{
		return HDR_TYPE_NONE;
	}
	virtual bool NeedsATICentroidHack() const
	{
		return false;
	}
	virtual bool SupportsColorOnSecondStream() const
	{
		return false;
	}
	virtual bool SupportsStaticPlusDynamicLighting() const
	{
		return false;
	}
	virtual bool SupportsStreamOffset() const
	{
		return false;
	}
	void SetDefaultDynamicState()
	{
	}
	virtual void CommitPixelShaderLighting( int pshReg )
	{
	}

	ShaderAPIOcclusionQuery_t CreateOcclusionQueryObject( void )
	{
		return INVALID_SHADERAPI_OCCLUSION_QUERY_HANDLE;
	}

	void DestroyOcclusionQueryObject( ShaderAPIOcclusionQuery_t handle )
	{
	}

	void BeginOcclusionQueryDrawing( ShaderAPIOcclusionQuery_t handle )
	{
	}

	void EndOcclusionQueryDrawing( ShaderAPIOcclusionQuery_t handle )
	{
	}

	int OcclusionQuery_GetNumPixelsRendered( ShaderAPIOcclusionQuery_t handle, bool bFlush )
	{
		return 0;
	}

	virtual void AcquireThreadOwnership();
	virtual void ReleaseThreadOwnership();

	virtual bool SupportsBorderColor() const { return false; }
	virtual bool SupportsFetch4() const { return false; }
	virtual bool CanStretchRectFromTextures( void ) const { return false; }
	virtual void EnableBuffer2FramesAhead( bool bEnable ) {}

	virtual void SetPSNearAndFarZ( int pshReg ) { }

	virtual void SetDepthFeatheringPixelShaderConstant( int iConstant, float fDepthBlendScale ) {}

	void SetPixelShaderFogParams( int reg )
	{
	}

	virtual bool InFlashlightMode() const
	{
		return false;
	}

	virtual bool InEditorMode() const
	{
		return false;
	}

	// What fields in the morph do we actually use?
	virtual MorphFormat_t ComputeMorphFormat( int numSnapshots, StateSnapshot_t* pIds ) const
	{
		return 0;
	}

	// Gets the bound morph's vertex format; returns 0 if no morph is bound
	virtual MorphFormat_t GetBoundMorphFormat()
	{
		return 0;
	}

	// Binds a standard texture
	virtual void BindStandardTexture( Sampler_t stage, StandardTextureId_t id )
	{
		switch ( id )
		{
		case TEXTURE_WHITE:
			BindTexture( stage, (ShaderAPITextureHandle_t)GetWhiteTexture() );
			break;
		case TEXTURE_BLACK:
			BindTexture( stage, (ShaderAPITextureHandle_t)GetBlackTexture() );
			break;
		case TEXTURE_GREY:
		case TEXTURE_GREY_ALPHA_ZERO:
			BindTexture( stage, (ShaderAPITextureHandle_t)GetGreyTexture() );
			break;
		default:
			BindTexture( stage, (ShaderAPITextureHandle_t)GetWhiteTexture() );
			break;
		}
	}

	virtual void BindStandardVertexTexture( VertexTextureSampler_t stage, StandardTextureId_t id )
	{
	}

	virtual void GetStandardTextureDimensions( int *pWidth, int *pHeight, StandardTextureId_t id )
	{
		*pWidth = *pHeight = 0;
	}


	virtual void SetFlashlightState( const FlashlightState_t &state, const VMatrix &worldToTexture )
	{
	}

	virtual void SetFlashlightStateEx( const FlashlightState_t &state, const VMatrix &worldToTexture, ITexture *pFlashlightDepthTexture )
	{
	}

	virtual const FlashlightState_t &GetFlashlightState( VMatrix &worldToTexture ) const
	{
		static FlashlightState_t  blah;
		return blah;
	}

	virtual const FlashlightState_t &GetFlashlightStateEx( VMatrix &worldToTexture, ITexture **pFlashlightDepthTexture ) const
	{
		static FlashlightState_t  blah;
		return blah;
	}

	virtual void ClearVertexAndPixelShaderRefCounts()
	{
	}

	virtual void PurgeUnusedVertexAndPixelShaders()
	{
	}

	virtual bool IsAAEnabled() const
	{
		return false;
	}

	virtual int GetVertexTextureCount() const
	{
		return 0;
	}

	virtual int GetMaxVertexTextureDimension() const
	{
		return 0;
	}

	virtual int  MaxTextureDepth() const
	{
		return 0;
	}

	// Binds a vertex texture to a particular texture stage in the vertex pipe
	virtual void BindVertexTexture( VertexTextureSampler_t nSampler, ShaderAPITextureHandle_t hTexture )
	{
	}

	// Sets morph target factors
	virtual void SetFlexWeights( int nFirstWeight, int nCount, const MorphWeight_t* pWeights )
	{
	}

	// NOTE: Stuff after this is added after shipping HL2.
	ITexture *GetRenderTargetEx( int nRenderTargetID )
	{
		return NULL;
	}

	void SetToneMappingScaleLinear( const Vector &scale )
	{
	}

	const Vector &GetToneMappingScaleLinear( void ) const
	{
		static Vector dummy;
		return dummy;
	}

	virtual float GetLightMapScaleFactor( void ) const
	{
		return 1.0;
	}


	// For dealing with device lost in cases where SwapBuffers isn't called all the time (Hammer)
	virtual void HandleDeviceLost()
	{
	}

	virtual void EnableLinearColorSpaceFrameBuffer( bool bEnable )
	{
	}

	// Lets the shader know about the full-screen texture so it can
	virtual void SetFullScreenTextureHandle( ShaderAPITextureHandle_t h )
	{
	}

	void SetFloatRenderingParameter(int parm_number, float value)
	{
	}

	void SetIntRenderingParameter(int parm_number, int value)
	{
	}
	void SetVectorRenderingParameter(int parm_number, Vector const &value)
	{
	}

	float GetFloatRenderingParameter(int parm_number) const
	{
		return 0;
	}

	int GetIntRenderingParameter(int parm_number) const
	{
		return 0;
	}

	Vector GetVectorRenderingParameter(int parm_number) const
	{
		return Vector(0,0,0);
	}

	// Methods related to stencil
	void SetStencilEnable(bool onoff)
	{
	}

	void SetStencilFailOperation(StencilOperation_t op)
	{
	}

	void SetStencilZFailOperation(StencilOperation_t op)
	{
	}

	void SetStencilPassOperation(StencilOperation_t op)
	{
	}

	void SetStencilCompareFunction(StencilComparisonFunction_t cmpfn)
	{
	}

	void SetStencilReferenceValue(int ref)
	{
	}

	void SetStencilTestMask(uint32 msk)
	{
	}

	void SetStencilWriteMask(uint32 msk)
	{
	}

	void ClearStencilBufferRectangle( int xmin, int ymin, int xmax, int ymax,int value)
	{
	}

	virtual void GetDXLevelDefaults(uint &max_dxlevel,uint &recommended_dxlevel)
	{
		max_dxlevel=recommended_dxlevel=90;
	}

	virtual void GetMaxToRender( IMesh *pMesh, bool bMaxUntilFlush, int *pMaxVerts, int *pMaxIndices )
	{
		*pMaxVerts = 32768;
		*pMaxIndices = 32768;
	}

	// Returns the max possible vertices + indices to render in a single draw call
	virtual int GetMaxVerticesToRender( IMaterial *pMaterial )
	{
		return 32768;
	}

	virtual int GetMaxIndicesToRender( )
	{
		return 32768;
	}
	virtual int CompareSnapshots( StateSnapshot_t snapshot0, StateSnapshot_t snapshot1 ) { return 0; }

	virtual void DisableAllLocalLights() {}

	virtual bool SupportsMSAAMode( int nMSAAMode ) { return false; }

	virtual bool SupportsCSAAMode( int nNumSamples, int nQualityLevel ) { return false; }

	// Hooks for firing PIX events from outside the Material System...
	virtual void BeginPIXEvent( unsigned long color, const char *szName ) {}
	virtual void EndPIXEvent() {}
	virtual void SetPIXMarker( unsigned long color, const char *szName ) {}

	virtual void ComputeVertexDescription( unsigned char* pBuffer, VertexFormat_t vertexFormat, MeshDesc_t& desc ) const
	{
		ComputeVertexDesc( pBuffer, vertexFormat, (VertexDesc_t&)desc );
	}

	virtual bool SupportsShadowDepthTextures() { return false; }

	virtual bool SupportsFetch4() { return false; }

	virtual int NeedsShaderSRGBConversion(void) const { return 0; }
	virtual bool UsesSRGBCorrectBlending() const { return false; }

	virtual bool HasFastVertexTextures() const { return false; }

	virtual void SetShadowDepthBiasFactors( float fShadowSlopeScaleDepthBias, float fShadowDepthBias ) {}

	virtual void SetDisallowAccess( bool ) {}
	virtual void EnableShaderShaderMutex( bool ) {}
	virtual void ShaderLock() {}
	virtual void ShaderUnlock() {}

// ------------ New Vertex/Index Buffer interface ----------------------------
	void BindVertexBuffer( int streamID, IVertexBuffer *pVertexBuffer, int nOffsetInBytes, int nFirstVertex, int nVertexCount, VertexFormat_t fmt, int nRepetitions1 );
	void BindIndexBuffer( IIndexBuffer *pIndexBuffer, int nOffsetInBytes );
	void Draw( MaterialPrimitiveType_t primitiveType, int firstIndex, int numIndices );
// ------------ End ----------------------------

	virtual int  GetVertexBufferCompression( void ) const { return 0; };

	virtual bool ShouldWriteDepthToDestAlpha( void ) const { return false; };
	virtual bool SupportsHDRMode( HDRType_t nHDRMode ) const { return false; };
	virtual bool IsDX10Card() const { return false; };

	void PushDeformation( const DeformationBase_t *pDeformation )
	{
	}

	virtual void PopDeformation( )
	{
	}

	int GetNumActiveDeformations( ) const
	{
		return 0;
	}

	// for shaders to set vertex shader constants. returns a packed state which can be used to set the dynamic combo
	int GetPackedDeformationInformation( int nMaskOfUnderstoodDeformations,
										 float *pConstantValuesOut,
										 int nBufferSize,
										 int nMaximumDeformations,
										 int *pNumDefsOut ) const
	{
		*pNumDefsOut = 0;
		return 0;
	}

	void SetStandardTextureHandle(StandardTextureId_t,ShaderAPITextureHandle_t)
	{
	}

	virtual void ExecuteCommandBuffer( uint8 *pData )
	{
	}
	virtual bool GetHDREnabled( void ) const { return false; }
	virtual void SetHDREnabled( bool bEnable ) {}

	virtual void CopyRenderTargetToScratchTexture( ShaderAPITextureHandle_t srcRt, ShaderAPITextureHandle_t dstTex, Rect_t *pSrcRect = NULL, Rect_t *pDstRect = NULL )
	{
	}

	// Allows locking and unlocking of very specific surface types.
	virtual void LockRect( void** pOutBits, int* pOutPitch, ShaderAPITextureHandle_t texHandle, int mipmap, int x, int y, int w, int h, bool bWrite, bool bRead )
	{
	}

	virtual void UnlockRect( ShaderAPITextureHandle_t texHandle, int mipmap )
	{
	}

	virtual void TexLodClamp( int finest ) {}

	virtual void TexLodBias( float bias ) {}

	virtual void CopyTextureToTexture( ShaderAPITextureHandle_t srcTex, ShaderAPITextureHandle_t dstTex ) {}

	void PrintfVA( char *fmt, va_list vargs ) {}
	void Printf( const char *fmt, ... ) {}
	float Knob( char *knobname, float *setvalue = NULL ) { return 0.0f; };

public:
	const Vector4D& GetCurrentColor() const { return m_CurrentColor; }
	IMaterial* GetBoundMaterial() const { return m_pMaterial; }
	void ApplyMaterialWriteMasks( IMaterial *pMaterial, bool bAlphaModulation ) const;

private:
	enum
	{
		TRANSLUCENT = 0x1,
		ALPHATESTED = 0x2,
		VERTEX_AND_PIXEL_SHADERS = 0x4,
		DEPTHWRITE = 0x8,
	};

	struct SnapshotRecord_t
	{
		unsigned int m_Flags;
		bool m_bColorWrite;
		bool m_bAlphaWrite;
		bool m_bDepthTest;
		ShaderBlendFactor_t m_SrcBlend, m_DstBlend;
		VertexFormat_t m_VertexUsage;
	};

	CMeshGL m_Mesh;
	CMatrixStack m_MatrixStack[MATERIAL_MODEL_MAX + 1];
	MaterialMatrixMode_t m_MatrixMode;
	Vector4D m_CurrentColor;
	CUtlVector<SnapshotRecord_t> m_Snapshots;
	StateSnapshot_t m_CurrentSnapshot;
	ShaderViewport_t m_Viewport;
	IMaterial *m_pMaterial;
	IVertexBuffer *m_pBoundVertexBuffer[4];
	int m_nBoundVertexOffset[4];
	int m_nBoundFirstVertex[4];
	int m_nBoundVertexCount[4];
	VertexFormat_t m_BoundVertexFormat[4];
	IIndexBuffer *m_pBoundIndexBuffer;
	int m_nBoundIndexOffset;

	void EnableAlphaToCoverage() {} ;
	void DisableAlphaToCoverage() {} ;

	ImageFormat GetShadowDepthTextureFormat() { return IMAGE_FORMAT_UNKNOWN; };
	ImageFormat GetNullTextureFormat() { return IMAGE_FORMAT_UNKNOWN; };
};


//-----------------------------------------------------------------------------
// Class Factory
//-----------------------------------------------------------------------------

static CShaderAPIGL g_ShaderAPIGL;
static CShaderShadowGL g_ShaderShadowGL;

// FIXME: Remove; it's for backward compat with the materialsystem only for now
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIGL, IShaderAPI,
									SHADERAPI_INTERFACE_VERSION, g_ShaderAPIGL )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderShadowGL, IShaderShadow,
								SHADERSHADOW_INTERFACE_VERSION, g_ShaderShadowGL )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIGL, IMaterialSystemHardwareConfig,
				MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, g_ShaderAPIGL )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIGL, IDebugTextureInfo,
				DEBUG_TEXTURE_INFO_VERSION, g_ShaderAPIGL )


//-----------------------------------------------------------------------------
// The main GL Shader util interface
//-----------------------------------------------------------------------------
IShaderUtil* g_pShaderUtil;


//-----------------------------------------------------------------------------
// Factory to return from SetMode
//-----------------------------------------------------------------------------
static void* ShaderInterfaceFactory( const char *pInterfaceName, int *pReturnCode )
{
	if ( pReturnCode )
	{
		*pReturnCode = IFACE_OK;
	}
	if ( !Q_stricmp( pInterfaceName, SHADER_DEVICE_INTERFACE_VERSION ) )
		return static_cast< IShaderDevice* >( &g_ShaderDeviceGL );
	if ( !Q_stricmp( pInterfaceName, SHADERAPI_INTERFACE_VERSION ) )
		return static_cast< IShaderAPI* >( &g_ShaderAPIGL );
	if ( !Q_stricmp( pInterfaceName, SHADERSHADOW_INTERFACE_VERSION ) )
		return static_cast< IShaderShadow* >( &g_ShaderShadowGL );

	if ( pReturnCode )
	{
		*pReturnCode = IFACE_FAILED;
	}
	return NULL;
}


//-----------------------------------------------------------------------------
//
// CShaderDeviceMgrGL
//
//-----------------------------------------------------------------------------
static ILauncherMgr *g_pLauncherMgr = NULL;

bool CShaderDeviceMgrGL::Connect( CreateInterfaceFn factory )
{
	// So others can access it
	g_pShaderUtil = (IShaderUtil*)factory( SHADER_UTIL_INTERFACE_VERSION, NULL );
#if defined( USE_SDL )
	g_pLauncherMgr = (ILauncherMgr*)factory( "SDLMgrInterface001", NULL );
#endif

	return true;
}

void CShaderDeviceMgrGL::Disconnect()
{
	g_pShaderUtil = NULL;
	g_pLauncherMgr = NULL;
}

void *CShaderDeviceMgrGL::QueryInterface( const char *pInterfaceName )
{
	if ( !Q_stricmp( pInterfaceName, SHADER_DEVICE_MGR_INTERFACE_VERSION ) )
		return static_cast< IShaderDeviceMgr* >( this );
	if ( !Q_stricmp( pInterfaceName, MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION ) )
		return static_cast< IMaterialSystemHardwareConfig* >( &g_ShaderAPIGL );
	return NULL;
}

InitReturnVal_t CShaderDeviceMgrGL::Init()
{
	return INIT_OK;
}

void CShaderDeviceMgrGL::Shutdown()
{

}

// Sets the adapter
bool CShaderDeviceMgrGL::SetAdapter( int nAdapter, int nFlags )
{
	return true;
}

// FIXME: Is this a public interface? Might only need to be private to shaderapi
CreateInterfaceFn CShaderDeviceMgrGL::SetMode( void *hWnd, int nAdapter, const ShaderDeviceInfo_t& mode )
{
	return ShaderInterfaceFactory;
}

// Gets the number of adapters...
int	 CShaderDeviceMgrGL::GetAdapterCount() const
{
	return 1;
}

bool CShaderDeviceMgrGL::GetRecommendedConfigurationInfo( int nAdapter, int nDXLevel, KeyValues *pKeyValues )
{
	return true;
}

// Returns info about each adapter
void CShaderDeviceMgrGL::GetAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const
{
	memset( &info, 0, sizeof( info ) );
	Q_strncpy( info.m_pDriverName, "OpenGL Native", sizeof(info.m_pDriverName) );
	info.m_VendorID = 0x10DE;
	info.m_DeviceID = 0x1000;
	info.m_nDXSupportLevel = 90;
	info.m_nMaxDXSupportLevel = 90;
}

// Returns the number of modes
int	 CShaderDeviceMgrGL::GetModeCount( int nAdapter ) const
{
	return 1;
}

// Returns mode information..
void CShaderDeviceMgrGL::GetModeInfo( ShaderDisplayMode_t *pInfo, int nAdapter, int nMode ) const
{
	if ( pInfo )
	{
		int w = 1280, h = 720;
		g_ShaderDeviceGL.GetBackBufferDimensions( w, h );
		pInfo->m_nWidth = w;
		pInfo->m_nHeight = h;
		pInfo->m_Format = IMAGE_FORMAT_RGB888;
		pInfo->m_nRefreshRateNumerator = 60;
		pInfo->m_nRefreshRateDenominator = 1;
	}
}

void CShaderDeviceMgrGL::GetCurrentModeInfo( ShaderDisplayMode_t* pInfo, int nAdapter ) const
{
	GetModeInfo( pInfo, nAdapter, 0 );
}


//-----------------------------------------------------------------------------
//
// Shader device empty
//
//-----------------------------------------------------------------------------
void CShaderDeviceGL::GetWindowSize( int &width, int &height ) const
{
	GetBackBufferDimensions( width, height );
}

void CShaderDeviceGL::GetBackBufferDimensions( int& width, int& height ) const
{
#if defined( USE_SDL )
	if ( g_pLauncherMgr )
	{
		uint w = 0, h = 0;
		g_pLauncherMgr->DisplayedSize( w, h );
		if ( w > 0 && h > 0 )
		{
			width = (int)w;
			height = (int)h;
			return;
		}
	}
	SDL_Window *pWin = SDL_GL_GetCurrentWindow();
	if ( pWin )
	{
		int w = 0, h = 0;
		SDL_GetWindowSize( pWin, &w, &h );
		if ( w > 0 && h > 0 )
		{
			width = w;
			height = h;
			return;
		}
	}
#endif
#if defined( ANDROID )
	EGLDisplay display = eglGetCurrentDisplay();
	EGLSurface surface = eglGetCurrentSurface( EGL_DRAW );
	if ( display != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE )
	{
		EGLint w = 0, h = 0;
		eglQuerySurface( display, surface, EGL_WIDTH, &w );
		eglQuerySurface( display, surface, EGL_HEIGHT, &h );
		if ( w > 0 && h > 0 )
		{
			width = (int)w;
			height = (int)h;
			return;
		}
	}
#endif
	width = 1280;
	height = 720;
}

// Use this to spew information about the 3D layer
void CShaderDeviceGL::SpewDriverInfo() const
{
	const char *pszVendor = (const char *)glGetString( GL_VENDOR );
	const char *pszRenderer = (const char *)glGetString( GL_RENDERER );
	const char *pszVersion = (const char *)glGetString( GL_VERSION );
	Msg( "OpenGL Vendor: %s\n", pszVendor ? pszVendor : "Unknown" );
	Msg( "OpenGL Renderer: %s\n", pszRenderer ? pszRenderer : "Unknown" );
	Msg( "OpenGL Version: %s\n", pszVersion ? pszVersion : "Unknown" );
}

// Creates/ destroys a child window
bool CShaderDeviceGL::AddView( void* hwnd )
{
	return true;
}

void CShaderDeviceGL::RemoveView( void* hwnd )
{
}

// Activates a view
void CShaderDeviceGL::SetView( void* hwnd )
{
}

void CShaderDeviceGL::ReleaseResources()
{
}

void CShaderDeviceGL::ReacquireResources()
{
}

// Creates/destroys Mesh
IMesh* CShaderDeviceGL::CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial * pMaterial )
{
	CMeshGL *pMesh = new CMeshGL( false );
	pMesh->SetVertexFormat( fmt );
	pMesh->SetMaterial( pMaterial );
	return pMesh;
}

void CShaderDeviceGL::DestroyStaticMesh( IMesh* mesh )
{
	if ( mesh && mesh != &m_Mesh && mesh != &m_DynamicMesh )
	{
		delete static_cast<CMeshGL*>( mesh );
	}
}

// Creates/destroys static vertex + index buffers
IVertexBuffer *CShaderDeviceGL::CreateVertexBuffer( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char *pTextureBudgetGroup )
{
	bool bDynamic = ( type == SHADER_BUFFER_TYPE_DYNAMIC || type == SHADER_BUFFER_TYPE_DYNAMIC_TEMP );
	CMeshGL *pMesh = new CMeshGL( bDynamic );
	pMesh->SetVertexFormat( fmt );
	return pMesh;
}

void CShaderDeviceGL::DestroyVertexBuffer( IVertexBuffer *pVertexBuffer )
{
	if ( pVertexBuffer && pVertexBuffer != &m_Mesh && pVertexBuffer != &m_DynamicMesh )
	{
		delete static_cast<CMeshGL*>( pVertexBuffer );
	}
}

IIndexBuffer *CShaderDeviceGL::CreateIndexBuffer( ShaderBufferType_t bufferType, MaterialIndexFormat_t fmt, int nIndexCount, const char *pTextureBudgetGroup )
{
	bool bDynamic = ( bufferType == SHADER_BUFFER_TYPE_DYNAMIC || bufferType == SHADER_BUFFER_TYPE_DYNAMIC_TEMP );
	CMeshGL *pMesh = new CMeshGL( bDynamic );
	return pMesh;
}

void CShaderDeviceGL::DestroyIndexBuffer( IIndexBuffer *pIndexBuffer )
{
	if ( pIndexBuffer && pIndexBuffer != &m_Mesh && pIndexBuffer != &m_DynamicMesh )
	{
		delete static_cast<CMeshGL*>( pIndexBuffer );
	}
}

IVertexBuffer *CShaderDeviceGL::GetDynamicVertexBuffer( int streamID, VertexFormat_t vertexFormat, bool bBuffered )
{
	m_DynamicMesh.SetVertexFormat( vertexFormat );
	return &m_DynamicMesh;
}

IIndexBuffer *CShaderDeviceGL::GetDynamicIndexBuffer( MaterialIndexFormat_t fmt, bool bBuffered )
{
	return &m_DynamicMesh;
}



//-----------------------------------------------------------------------------
//
// The empty mesh...
//
//-----------------------------------------------------------------------------
CMeshGL::CMeshGL( bool bIsDynamic ) : m_bIsDynamic( bIsDynamic )
{
	m_pVertexMemory = NULL;
	m_nVertexAllocSize = 0;
	m_pIndexMemory = NULL;
	m_nIndexAllocSize = 0;
	m_Type = MATERIAL_TRIANGLES;
	m_VertexFormat = 0;
	m_pMaterial = NULL;
	m_nIndexCount = 0;
	m_nVertexCount = 0;
	m_hVBO = 0;
	m_hIBO = 0;
	m_bVBOValid = false;
	m_bIBOValid = false;
	m_pVertexOverride = NULL;
	m_pIndexOverride = NULL;

	if ( bIsDynamic )
	{
		EnsureVertexAllocation( 1024 * 1024 );
		EnsureIndexAllocation( 512 * 1024 );
	}
}

CMeshGL::~CMeshGL()
{
	if ( m_hVBO )
	{
		glDeleteBuffers( 1, &m_hVBO );
		m_hVBO = 0;
	}
	if ( m_hIBO )
	{
		glDeleteBuffers( 1, &m_hIBO );
		m_hIBO = 0;
	}
	delete[] m_pVertexMemory;
	m_pVertexMemory = NULL;
	delete[] m_pIndexMemory;
	m_pIndexMemory = NULL;
}

void CMeshGL::EnsureVertexAllocation( int nBytes )
{
	if ( nBytes <= 0 )
		return;
	if ( nBytes > m_nVertexAllocSize )
	{
		int newSize = ( m_bIsDynamic && ( m_nVertexAllocSize * 2 > nBytes ) ) ? ( m_nVertexAllocSize * 2 ) : nBytes;
		if ( newSize < 1024 )
			newSize = 1024;
		unsigned char *pNew = new unsigned char[newSize];
		memset( pNew, 0, newSize );
		if ( m_pVertexMemory && m_nVertexCount > 0 )
		{
			int copySize = ( m_nVertexAllocSize < newSize ) ? m_nVertexAllocSize : newSize;
			memcpy( pNew, m_pVertexMemory, copySize );
		}
		delete[] m_pVertexMemory;
		m_pVertexMemory = pNew;
		m_nVertexAllocSize = newSize;
		m_bVBOValid = false;
		m_bIBOValid = false;
	}
}

void CMeshGL::EnsureIndexAllocation( int nBytes )
{
	if ( nBytes <= 0 )
		return;
	if ( nBytes > m_nIndexAllocSize )
	{
		int newSize = ( m_bIsDynamic && ( m_nIndexAllocSize * 2 > nBytes ) ) ? ( m_nIndexAllocSize * 2 ) : nBytes;
		if ( newSize < 1024 )
			newSize = 1024;
		unsigned char *pNew = new unsigned char[newSize];
		memset( pNew, 0, newSize );
		if ( m_pIndexMemory && m_nIndexCount > 0 )
		{
			int copySize = ( m_nIndexAllocSize < newSize ) ? m_nIndexAllocSize : newSize;
			memcpy( pNew, m_pIndexMemory, copySize );
		}
		delete[] m_pIndexMemory;
		m_pIndexMemory = pNew;
		m_nIndexAllocSize = newSize;
		m_bVBOValid = false;
		m_bIBOValid = false;
	}
}

bool CMeshGL::Lock( int nMaxIndexCount, bool bAppend, IndexDesc_t& desc )
{
	int firstIndex = bAppend ? m_nIndexCount : 0;
	int count = ( nMaxIndexCount > 0 ) ? ( nMaxIndexCount + 16 ) : 32;
	int neededBytes = ( firstIndex + count ) * sizeof(unsigned short);
	EnsureIndexAllocation( neededBytes );

	desc.m_pIndices = (unsigned short*)( m_pIndexMemory + firstIndex * sizeof(unsigned short) );
	desc.m_nIndexSize = 1;
	desc.m_nFirstIndex = firstIndex;
	desc.m_nOffset = firstIndex * sizeof(unsigned short);
	return true;
}

void CMeshGL::Unlock( int nWrittenIndexCount, IndexDesc_t& desc )
{
	m_nIndexCount = desc.m_nFirstIndex + nWrittenIndexCount;
	m_bVBOValid = false;
	m_bIBOValid = false;
}

void CMeshGL::ModifyBegin( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t& desc )
{
	int count = nFirstIndex + nIndexCount + 16;
	int neededBytes = count * sizeof(unsigned short);
	EnsureIndexAllocation( neededBytes );

	desc.m_pIndices = (unsigned short*)( m_pIndexMemory + nFirstIndex * sizeof(unsigned short) );
	desc.m_nIndexSize = 1;
	desc.m_nFirstIndex = nFirstIndex;
	desc.m_nOffset = nFirstIndex * sizeof(unsigned short);
}

void CMeshGL::ModifyEnd( IndexDesc_t& desc )
{
	m_bVBOValid = false;
	m_bIBOValid = false;
}

void CMeshGL::Spew( int nIndexCount, const IndexDesc_t & desc )
{
}

void CMeshGL::ValidateData( int nIndexCount, const IndexDesc_t &desc )
{
}

bool CMeshGL::Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc )
{
	if ( m_VertexFormat == 0 )
	{
		m_VertexFormat = ComputeGLVertexFormat( VERTEX_POSITION | VERTEX_COLOR, 1, NULL, 0, 0 );
	}

	VertexDesc_t tempDesc;
	ComputeVertexDesc( NULL, m_VertexFormat, tempDesc );
	int stride = tempDesc.m_ActualVertexSize > 0 ? tempDesc.m_ActualVertexSize : 64;

	int firstVertex = bAppend ? m_nVertexCount : 0;
	int count = ( nVertexCount > 0 ) ? ( nVertexCount + 16 ) : 32;
	int neededBytes = ( firstVertex + count ) * stride;
	EnsureVertexAllocation( neededBytes );

	ComputeVertexDesc( m_pVertexMemory + firstVertex * stride, m_VertexFormat, desc );
	desc.m_nFirstVertex = firstVertex;
	desc.m_nOffset = firstVertex * stride;
	return true;
}

void CMeshGL::Unlock( int nVertexCount, VertexDesc_t &desc )
{
	m_nVertexCount = desc.m_nFirstVertex + nVertexCount;
	m_bVBOValid = false;
	m_bIBOValid = false;
}

void CMeshGL::Spew( int nVertexCount, const VertexDesc_t &desc )
{
}

void CMeshGL::ValidateData( int nVertexCount, const VertexDesc_t & desc )
{
}

void CMeshGL::LockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	if ( numVerts > 0 ) m_pVertexOverride = NULL;
	if ( numIndices > 0 ) m_pIndexOverride = NULL;
	Lock( numVerts, false, *static_cast<VertexDesc_t*>( &desc ) );
	Lock( numIndices, false, *static_cast<IndexDesc_t*>( &desc ) );
}

void CMeshGL::UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	m_nVertexCount = numVerts;
	m_nIndexCount = numIndices;
	m_bVBOValid = false;
	m_bIBOValid = false;
}

void CMeshGL::ModifyBeginEx( bool bReadOnly, int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc )
{
	VertexDesc_t layout;
	ComputeVertexDesc( NULL, m_VertexFormat, layout );
	EnsureVertexAllocation( ( firstVertex + numVerts ) * layout.m_ActualVertexSize );
	ComputeVertexDesc( m_pVertexMemory + firstVertex * layout.m_ActualVertexSize, m_VertexFormat,
		*static_cast<VertexDesc_t*>( &desc ) );
	desc.m_nFirstVertex = firstVertex;
	static_cast<VertexDesc_t*>( &desc )->m_nOffset = firstVertex * layout.m_ActualVertexSize;
	ModifyBegin( bReadOnly, firstIndex, numIndices, *static_cast<IndexDesc_t*>( &desc ) );
}

void CMeshGL::ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc )
{
	ModifyBeginEx( false, firstVertex, numVerts, firstIndex, numIndices, desc );
}

void CMeshGL::ModifyEnd( MeshDesc_t& desc )
{
	m_bVBOValid = false;
	m_bIBOValid = false;
}

int CMeshGL::VertexCount() const
{
	return m_nVertexCount;
}

void CMeshGL::SetPrimitiveType( MaterialPrimitiveType_t type )
{
	m_Type = type;
}

static GLuint s_hGLProgram = 0;
static GLint s_uMVP = -1;
static GLint s_uColor = -1;
static GLint s_uTexture = -1;
static GLint s_uLightmap = -1;
static GLint s_uUseTexture = -1;
static GLint s_uFlipTextureV = -1;
static GLint s_uShadow = -1;
static GLint s_uShadowColor = -1;
static void GetGLTargetSize( int &width, int &height );
static GLint s_uUseLightmap = -1;
static GLint s_uAlphaTest = -1;
static GLint s_uTranslucent = -1;
static GLint s_aPosition = -1;
static GLint s_aColor = -1;
static GLint s_aTexCoord = -1;
static GLint s_aLightCoord = -1;
static GLuint s_hVBO = 0;
static GLuint s_hIBO = 0;

static void EnsureBuffers()
{
	if ( s_hVBO == 0 )
	{
		glGenBuffers( 1, &s_hVBO );
		glGenBuffers( 1, &s_hIBO );
	}
}

static void EnsureGLProgram()
{
	if ( s_hGLProgram != 0 )
		return;

	const char *vshSource =
		"attribute vec3 a_position;\n"
		"attribute vec4 a_color;\n"
		"attribute vec2 a_texcoord;\n"
		"attribute vec2 a_lightcoord;\n"
		"uniform mat4 u_mvp;\n"
		"uniform vec4 u_color;\n"
		"varying vec4 v_color;\n"
		"varying vec2 v_texcoord;\n"
		"varying vec2 v_lightcoord;\n"
		"void main() {\n"
		"    gl_Position = u_mvp * vec4(a_position, 1.0);\n"
		"    v_color = a_color * u_color;\n"
		"    v_texcoord = a_texcoord;\n"
		"    v_lightcoord = a_lightcoord;\n"
		"}\n";

	const char *fshSource =
		"#ifdef GL_ES\n"
		"precision mediump float;\n"
		"#endif\n"
		"varying vec4 v_color;\n"
		"varying vec2 v_texcoord;\n"
		"varying vec2 v_lightcoord;\n"
		"uniform sampler2D u_texture;\n"
		"uniform sampler2D u_lightmap;\n"
		"uniform int u_use_texture;\n"
		"uniform int u_flip_texture_v;\n"
		"uniform int u_use_lightmap;\n"
		"uniform int u_alphatest;\n"
		"uniform int u_translucent;\n"
		"uniform int u_shadow;\n"
		"uniform vec3 u_shadow_color;\n"
		"void main() {\n"
		"    vec2 uv = vec2(v_texcoord.x, (u_flip_texture_v != 0) ? 1.0 - v_texcoord.y : v_texcoord.y);\n"
		"    vec4 texel = (u_use_texture != 0) ? texture2D(u_texture, uv) : vec4(1.0);\n"
		"    if (u_shadow != 0) {\n"
		"        float coverage = (u_use_texture != 0) ? clamp(texel.a - v_color.a, 0.0, 1.0) : 0.0;\n"
		"        gl_FragColor = vec4(mix(vec3(1.0), u_shadow_color, coverage), 1.0);\n"
		"        return;\n"
		"    }\n"
		"    vec4 col = texel * v_color;\n"
		"    if (u_use_lightmap != 0) {\n"
		"        vec4 light = texture2D(u_lightmap, v_lightcoord);\n"
		"        col.rgb *= light.rgb * 2.0;\n"
		"    }\n"
		"    if (u_alphatest != 0) {\n"
		"        if (col.a < 0.3) discard;\n"
		"    } else if (u_translucent == 0) {\n"
		"        col.a = 1.0;\n"
		"    }\n"
		"    gl_FragColor = col;\n"
		"}\n";

	GLuint vsh = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( vsh, 1, &vshSource, NULL );
	glCompileShader( vsh );

	GLint compiled = 0;
	glGetShaderiv( vsh, GL_COMPILE_STATUS, &compiled );
	if ( !compiled )
	{
		char infoLog[1024] = {0};
		glGetShaderInfoLog( vsh, sizeof(infoLog), NULL, infoLog );
		Warning( "[GL] Vertex shader compile failed: %s\n", infoLog );
	}

	GLuint fsh = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( fsh, 1, &fshSource, NULL );
	glCompileShader( fsh );

	compiled = 0;
	glGetShaderiv( fsh, GL_COMPILE_STATUS, &compiled );
	if ( !compiled )
	{
		char infoLog[1024] = {0};
		glGetShaderInfoLog( fsh, sizeof(infoLog), NULL, infoLog );
		Warning( "[GL] Fragment shader compile failed: %s\n", infoLog );
	}

	s_hGLProgram = glCreateProgram();
	glAttachShader( s_hGLProgram, vsh );
	glAttachShader( s_hGLProgram, fsh );
	glLinkProgram( s_hGLProgram );

	GLint linked = 0;
	glGetProgramiv( s_hGLProgram, GL_LINK_STATUS, &linked );
	if ( !linked )
	{
		char infoLog[1024] = {0};
		glGetProgramInfoLog( s_hGLProgram, sizeof(infoLog), NULL, infoLog );
		Warning( "[GL] Shader program link failed: %s\n", infoLog );
	}

	glDeleteShader( vsh );
	glDeleteShader( fsh );

	s_uMVP = glGetUniformLocation( s_hGLProgram, "u_mvp" );
	s_uColor = glGetUniformLocation( s_hGLProgram, "u_color" );
	s_uTexture = glGetUniformLocation( s_hGLProgram, "u_texture" );
	s_uLightmap = glGetUniformLocation( s_hGLProgram, "u_lightmap" );
	s_uUseTexture = glGetUniformLocation( s_hGLProgram, "u_use_texture" );
	s_uFlipTextureV = glGetUniformLocation( s_hGLProgram, "u_flip_texture_v" );
	s_uUseLightmap = glGetUniformLocation( s_hGLProgram, "u_use_lightmap" );
	s_uAlphaTest = glGetUniformLocation( s_hGLProgram, "u_alphatest" );
	s_uTranslucent = glGetUniformLocation( s_hGLProgram, "u_translucent" );
	s_uShadow = glGetUniformLocation( s_hGLProgram, "u_shadow" );
	s_uShadowColor = glGetUniformLocation( s_hGLProgram, "u_shadow_color" );
	s_aPosition = glGetAttribLocation( s_hGLProgram, "a_position" );
	s_aColor = glGetAttribLocation( s_hGLProgram, "a_color" );
	s_aTexCoord = glGetAttribLocation( s_hGLProgram, "a_texcoord" );
	s_aLightCoord = glGetAttribLocation( s_hGLProgram, "a_lightcoord" );

	Msg( "[GL] Shader program initialized: prog=%u, u_mvp=%d, a_pos=%d, a_col=%d, a_tc=%d, a_lc=%d\n",
		(unsigned int)s_hGLProgram, s_uMVP, s_aPosition, s_aColor, s_aTexCoord, s_aLightCoord );
}

static bool BindGLMaterialTexture( IMaterial *pMaterial, bool *pFlipV = NULL )
{
	if ( pFlipV ) *pFlipV = false;
	if ( !pMaterial ) return false;
	const char *pShader = pMaterial->GetShaderName();
	// Engine_Post combines bloom with the full scene. Until that shader is
	// implemented, copy the full scene instead of displaying the bloom buffer.
	bool bPost = pShader && !Q_strnicmp( pShader, "Engine_Post", 11 );
	const char *names[] = { bPost ? "$fbtexture" : "$basetexture", "$hdrbasetexture" };
	for ( int i = 0; i < 2; ++i )
	{
		bool found = false;
		IMaterialVar *pVar = pMaterial->FindVar( names[i], &found, false );
		if ( !found || !pVar || pVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE ) continue;
		ITexture *pTexture = pVar->GetTextureValue();
		if ( !pTexture ) continue;
		static_cast<ITextureInternal*>( pTexture )->Bind( SHADER_SAMPLER0 );
		if ( pFlipV ) *pFlipV = pTexture->IsRenderTarget();
		return true;
	}
	return false;
}

// Brush renderers pass distance fading through IMaterial::AlphaModulate.
// UI already supplies its draw color; projected shadows use vertex alpha as distance.
static float GetGLMaterialAlpha( IMaterial *pMaterial, bool bIs2D, bool bShadow )
{
	return ( pMaterial && !bIs2D && !bShadow ) ? pMaterial->GetAlphaModulation() : 1.0f;
}

static int GetGLAlphaSnapshotIndex( ShaderRenderState_t *pState, bool bAlphaModulation )
{
	int modulation = bAlphaModulation ? SHADER_USING_ALPHA_MODULATION : 0;
	return pState->m_pSnapshots[modulation].m_nPassCount > 0 ? modulation : 0;
}

static IMaterial *ResolveGLDrawMaterial( IMaterial *pBoundMaterial, IMaterial *pMeshMaterial, IMaterial *pVertexMaterial )
{
	if ( pBoundMaterial ) return pBoundMaterial;
	return pMeshMaterial ? pMeshMaterial : pVertexMaterial;
}

static void DrawMeshInternal( CMeshGL *pVB, CMeshGL *pIB, MaterialPrimitiveType_t primType, int firstIndex, int numIndices, IMaterial *pOverrideMat, int nVertexOffset = 0 )
{
	if ( !pVB || !pVB->m_pVertexMemory || pVB->m_nVertexCount <= 0 )
		return;

	if ( !pIB )
		pIB = pVB;

	if ( firstIndex < 0 )
		firstIndex = 0;

	if ( numIndices <= 0 )
	{
		numIndices = pIB ? pIB->m_nIndexCount : pVB->m_nIndexCount;
	}

	if ( numIndices <= 0 && pVB->m_nVertexCount <= 0 )
		return;

	EnsureGLProgram();
	glUseProgram( s_hGLProgram );

	// MaterialSystem defers matrix uploads until a mesh is drawn.
	g_pShaderUtil->SyncMatrices();
	VMatrix matProj, matView, matModel;
	g_ShaderAPIGL.GetMatrix( MATERIAL_PROJECTION, matProj.Base() );
	g_ShaderAPIGL.GetMatrix( MATERIAL_VIEW, matView.Base() );
	g_ShaderAPIGL.GetMatrix( MATERIAL_MODEL, matModel.Base() );

	VMatrix modelView, mvp;
	MatrixMultiply( matModel, matView, modelView );
	MatrixMultiply( modelView, matProj, mvp );

	if ( s_uMVP >= 0 )
	{
		glUniformMatrix4fv( s_uMVP, 1, GL_FALSE, mvp.Base() );
	}

	bool bIs2D = ( matProj[3][3] != 0.0f );
	Vector4D curColor = bIs2D ? g_ShaderAPIGL.GetCurrentColor() : Vector4D( 1.0f, 1.0f, 1.0f, 1.0f );
	// World batches reuse one mesh while Bind changes for each surface group.
	IMaterial *pMat = ResolveGLDrawMaterial( g_ShaderAPIGL.GetBoundMaterial(), pOverrideMat, pVB->m_pMaterial );

	const char *pShaderName = pMat ? pMat->GetShaderName() : NULL;
	bool bShadow = pShaderName && ( !Q_stricmp( pShaderName, "Shadow" ) ||
		!Q_stricmp( pShaderName, "Shadow_DX8" ) || !Q_stricmp( pShaderName, "Shadow_DX6" ) );
	float materialAlpha = GetGLMaterialAlpha( pMat, bIs2D, bShadow );
	curColor.w *= materialAlpha;
	if ( s_uColor >= 0 )
		glUniform4f( s_uColor, curColor.x, curColor.y, curColor.z, curColor.w );
	glUniform1i( s_uShadow, bShadow ? 1 : 0 );
	if ( bShadow )
	{
		float r, g, b;
		pMat->GetColorModulation( &r, &g, &b );
		glUniform3f( s_uShadowColor, r, g, b );
	}

	bool bFlipTextureV = false;
	bool bBoundTex = BindGLMaterialTexture( pMat, &bFlipTextureV );
	glUniform1i( s_uFlipTextureV, bFlipTextureV ? 1 : 0 );
	if ( !bBoundTex )
	{
		glActiveTexture( GL_TEXTURE0 );
		GLint curTex0 = 0;
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &curTex0 );
		if ( pMat || curTex0 == 0 )
		{
			glBindTexture( GL_TEXTURE_2D, GetWhiteTexture() );
		}
	}
	bool bLightmapped = false;
	if ( !bIs2D && pMat )
	{
		bool bFound = false;
		IMaterialVar *pFlags = pMat->FindVar( "$flags2", &bFound, false );
		bLightmapped = bFound && pFlags && ( pFlags->GetIntValue() & MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	}
	if ( bLightmapped )
		g_pShaderUtil->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );
	glActiveTexture( GL_TEXTURE1 );
	GLint curTex1 = 0;
	glGetIntegerv( GL_TEXTURE_BINDING_2D, &curTex1 );
	if ( curTex1 == 0 )
	{
		glBindTexture( GL_TEXTURE_2D, GetWhiteTexture() );
	}
	glActiveTexture( GL_TEXTURE0 );

	if ( s_uTexture >= 0 )
	{
		glUniform1i( s_uTexture, 0 );
	}
	if ( s_uLightmap >= 0 )
	{
		glUniform1i( s_uLightmap, 1 );
	}

	bool bAlphaTest = pMat ? pMat->GetMaterialVarFlag( MATERIAL_VAR_ALPHATEST ) : false;
	bool bTranslucent = pMat ? pMat->IsTranslucent() : false;
	bool bAdditive = pMat ? pMat->GetMaterialVarFlag( MATERIAL_VAR_ADDITIVE ) : false;
	bool bIgnoreZ = pMat ? pMat->GetMaterialVarFlag( MATERIAL_VAR_IGNOREZ ) : false;

	if ( s_uAlphaTest >= 0 )
	{
		glUniform1i( s_uAlphaTest, bAlphaTest ? 1 : 0 );
	}
	if ( s_uTranslucent >= 0 )
	{
		glUniform1i( s_uTranslucent, ( bTranslucent || bAdditive || bIs2D ) ? 1 : 0 );
	}

	if ( pVB->m_VertexFormat == 0 )
	{
		pVB->m_VertexFormat = ComputeGLVertexFormat( VERTEX_POSITION | VERTEX_COLOR, 1, NULL, 0, 0 );
	}

	VertexDesc_t desc;
	ComputeVertexDesc( pVB->m_pVertexMemory, pVB->m_VertexFormat, desc );
	if ( desc.m_ActualVertexSize <= 0 )
	{
		desc.m_ActualVertexSize = 32;
	}

	GLuint curVBO = 0;
	GLuint curIBO = 0;

	if ( !pVB->m_bIsDynamic )
	{
		if ( !pVB->m_bVBOValid || pVB->m_hVBO == 0 )
		{
			if ( pVB->m_hVBO == 0 ) glGenBuffers( 1, &pVB->m_hVBO );
			int nVertexBufferSize = pVB->m_nVertexCount * desc.m_ActualVertexSize;
			if ( pVB->m_nVertexAllocSize > 0 && nVertexBufferSize > pVB->m_nVertexAllocSize )
			{
				nVertexBufferSize = pVB->m_nVertexAllocSize;
			}
			if ( nVertexBufferSize > 0 && pVB->m_pVertexMemory )
			{
				glBindBuffer( GL_ARRAY_BUFFER, pVB->m_hVBO );
				glBufferData( GL_ARRAY_BUFFER, nVertexBufferSize, pVB->m_pVertexMemory, GL_STATIC_DRAW );
			}
			pVB->m_bVBOValid = true;
		}
		curVBO = pVB->m_hVBO;
	}
	else
	{
		EnsureBuffers();
		curVBO = s_hVBO;

		int nVertexBufferSize = pVB->m_nVertexCount * desc.m_ActualVertexSize;
		if ( pVB->m_nVertexAllocSize > 0 && nVertexBufferSize > pVB->m_nVertexAllocSize )
		{
			nVertexBufferSize = pVB->m_nVertexAllocSize;
		}
		if ( nVertexBufferSize > 0 && pVB->m_pVertexMemory )
		{
			glBindBuffer( GL_ARRAY_BUFFER, s_hVBO );
			glBufferData( GL_ARRAY_BUFFER, nVertexBufferSize, pVB->m_pVertexMemory, GL_DYNAMIC_DRAW );
		}
	}

	if ( pIB && pIB->m_nIndexCount > 0 && pIB->m_pIndexMemory )
	{
		if ( !pIB->m_bIsDynamic )
		{
			if ( !pIB->m_bIBOValid || pIB->m_hIBO == 0 )
			{
				if ( pIB->m_hIBO == 0 ) glGenBuffers( 1, &pIB->m_hIBO );
				int maxIndices = pIB->m_nIndexCount;
				if ( pIB->m_nIndexAllocSize > 0 && maxIndices * (int)sizeof(unsigned short) > pIB->m_nIndexAllocSize )
				{
					maxIndices = pIB->m_nIndexAllocSize / sizeof(unsigned short);
				}
				if ( maxIndices > 0 )
				{
					glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pIB->m_hIBO );
					glBufferData( GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(unsigned short), pIB->m_pIndexMemory, GL_STATIC_DRAW );
				}
				pIB->m_bIBOValid = true;
			}
			curIBO = pIB->m_hIBO;
		}
		else
		{
			EnsureBuffers();
			curIBO = s_hIBO;
			int uploadCount = pIB->m_nIndexCount;
			if ( pIB->m_nIndexAllocSize > 0 && uploadCount * (int)sizeof(unsigned short) > pIB->m_nIndexAllocSize )
			{
				uploadCount = pIB->m_nIndexAllocSize / sizeof(unsigned short);
			}
			if ( uploadCount > 0 )
			{
				glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_hIBO );
				glBufferData( GL_ELEMENT_ARRAY_BUFFER, uploadCount * sizeof(unsigned short), pIB->m_pIndexMemory, GL_DYNAMIC_DRAW );
			}
		}
	}

	if ( curVBO != 0 )
	{
		glBindBuffer( GL_ARRAY_BUFFER, curVBO );

		if ( s_aPosition >= 0 && desc.m_VertexSize_Position > 0 && desc.m_pPosition && pVB->m_pVertexMemory && (pVB->m_VertexFormat & VERTEX_POSITION) )
		{
			uintptr_t offset = (uintptr_t)desc.m_pPosition - (uintptr_t)pVB->m_pVertexMemory + nVertexOffset;
			glEnableVertexAttribArray( s_aPosition );
			glVertexAttribPointer( s_aPosition, 3, GL_FLOAT, GL_FALSE, desc.m_VertexSize_Position, (const void*)offset );
		}
		else if ( s_aPosition >= 0 )
		{
			glDisableVertexAttribArray( s_aPosition );
		}

		if ( s_aColor >= 0 && desc.m_VertexSize_Color > 0 && desc.m_pColor && pVB->m_pVertexMemory && (pVB->m_VertexFormat & VERTEX_COLOR) )
		{
			uintptr_t offset = (uintptr_t)desc.m_pColor - (uintptr_t)pVB->m_pVertexMemory + nVertexOffset;
			glEnableVertexAttribArray( s_aColor );
			glVertexAttribPointer( s_aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, desc.m_VertexSize_Color, (const void*)offset );
		}
		else if ( s_aColor >= 0 )
		{
			glDisableVertexAttribArray( s_aColor );
			glVertexAttrib4f( s_aColor, 1.0f, 1.0f, 1.0f, 1.0f );
		}

		bool bHasTexCoord = ( desc.m_VertexSize_TexCoord[0] > 0 && desc.m_pTexCoord[0] && pVB->m_pVertexMemory && (TexCoordSize( 0, pVB->m_VertexFormat ) > 0) );
		if ( s_aTexCoord >= 0 && bHasTexCoord )
		{
			uintptr_t offset = (uintptr_t)desc.m_pTexCoord[0] - (uintptr_t)pVB->m_pVertexMemory + nVertexOffset;
			glEnableVertexAttribArray( s_aTexCoord );
			glVertexAttribPointer( s_aTexCoord, 2, GL_FLOAT, GL_FALSE, desc.m_VertexSize_TexCoord[0], (const void*)offset );
			if ( s_uUseTexture >= 0 ) glUniform1i( s_uUseTexture, 1 );
		}
		else
		{
			if ( s_aTexCoord >= 0 ) glDisableVertexAttribArray( s_aTexCoord );
			if ( s_uUseTexture >= 0 ) glUniform1i( s_uUseTexture, 0 );
		}

		bool bHasLightCoord = bLightmapped && ( desc.m_VertexSize_TexCoord[1] > 0 && desc.m_pTexCoord[1] && pVB->m_pVertexMemory && (TexCoordSize( 1, pVB->m_VertexFormat ) > 0) );
		if ( s_aLightCoord >= 0 && bHasLightCoord )
		{
			uintptr_t offset = (uintptr_t)desc.m_pTexCoord[1] - (uintptr_t)pVB->m_pVertexMemory + nVertexOffset;
			glEnableVertexAttribArray( s_aLightCoord );
			glVertexAttribPointer( s_aLightCoord, 2, GL_FLOAT, GL_FALSE, desc.m_VertexSize_TexCoord[1], (const void*)offset );
			if ( s_uUseLightmap >= 0 ) glUniform1i( s_uUseLightmap, 1 );
		}
		else
		{
			if ( s_aLightCoord >= 0 ) glDisableVertexAttribArray( s_aLightCoord );
			if ( s_uUseLightmap >= 0 ) glUniform1i( s_uUseLightmap, 0 );
		}
	}

	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

	if ( bIs2D || bIgnoreZ )
	{
		glDisable( GL_DEPTH_TEST );
		glDepthMask( GL_FALSE );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else if ( bTranslucent || bAdditive )
	{
		glEnable( GL_BLEND );
		if ( bAdditive )
			glBlendFunc( GL_SRC_ALPHA, GL_ONE );
		else
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		glDepthMask( GL_FALSE );
	}
	else
	{
		glDisable( GL_BLEND );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		glDepthMask( GL_TRUE );
	}
	g_ShaderAPIGL.ApplyMaterialWriteMasks( pMat, materialAlpha < 1.0f );
	glDisable( GL_CULL_FACE );

	GLenum mode = GL_TRIANGLES;
	switch ( primType )
	{
	case MATERIAL_LINES: mode = GL_LINES; break;
	case MATERIAL_LINE_STRIP: mode = GL_LINE_STRIP; break;
	case MATERIAL_POINTS: mode = GL_POINTS; break;
	case MATERIAL_TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
	default: mode = GL_TRIANGLES; break;
	}

	if ( pIB && pIB->m_nIndexCount > 0 && numIndices > 0 && curIBO != 0 )
	{
		if ( firstIndex + numIndices > pIB->m_nIndexCount )
		{
			numIndices = pIB->m_nIndexCount - firstIndex;
		}
		if ( numIndices > 0 )
		{
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, curIBO );
			glDrawElements( mode, numIndices, GL_UNSIGNED_SHORT, (const void *)( (uintptr_t)( firstIndex * sizeof(unsigned short) ) ) );
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
		}
	}
	else if ( pVB->m_nVertexCount > 0 )
	{
		glDrawArrays( mode, 0, pVB->m_nVertexCount );
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	if ( s_aPosition >= 0 ) glDisableVertexAttribArray( s_aPosition );
	if ( s_aColor >= 0 ) glDisableVertexAttribArray( s_aColor );
	if ( s_aTexCoord >= 0 ) glDisableVertexAttribArray( s_aTexCoord );
	if ( s_aLightCoord >= 0 ) glDisableVertexAttribArray( s_aLightCoord );

	static int s_nWorldDrawSamples = 0;
	if ( !bIs2D && s_nWorldDrawSamples < 16 )
	{
		++s_nWorldDrawSamples;
		Msg( "[GL] World draw %d: material=%s verts=%d first=%d count=%d lightmap=%d error=0x%x\n",
			s_nWorldDrawSamples, pMat ? pMat->GetName() : "<none>", pVB->m_nVertexCount,
			firstIndex, numIndices, bLightmapped ? 1 : 0, (unsigned int)glGetError() );
	}
}

void CMeshGL::Draw( int firstIndex, int numIndices )
{
	if ( !g_pShaderUtil->OnDrawMesh( this, firstIndex, numIndices ) )
		return;
	CMeshGL *pVB = m_pVertexOverride ? m_pVertexOverride : this;
	CMeshGL *pIB = m_pIndexOverride ? m_pIndexOverride : this;
	DrawMeshInternal( pVB, pIB, m_Type, firstIndex, numIndices, m_pMaterial );
}

void CMeshGL::Draw(CPrimList *pPrims, int nPrims)
{
	if ( !g_pShaderUtil->OnDrawMesh( this, pPrims, nPrims ) )
		return;
	if ( !pPrims || nPrims <= 0 )
		return;
	for ( int i = 0; i < nPrims; ++i )
	{
		DrawMeshInternal( m_pVertexOverride ? m_pVertexOverride : this,
			m_pIndexOverride ? m_pIndexOverride : this, m_Type,
			pPrims[i].m_FirstIndex, pPrims[i].m_NumIndices, m_pMaterial );
	}
}

// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
void CMeshGL::CopyToMeshBuilder(
	int iStartVert,		// Which vertices to copy.
	int nVerts,
	int iStartIndex,	// Which indices to copy.
	int nIndices,
	int indexOffset,	// This is added to each index.
	CMeshBuilder &builder )
{
}

// Spews the mesh data
void CMeshGL::Spew( int numVerts, int numIndices, const MeshDesc_t & desc )
{
}

void CMeshGL::ValidateData( int numVerts, int numIndices, const MeshDesc_t & desc )
{
}

// gets the associated material
IMaterial* CMeshGL::GetMaterial()
{
	return m_pMaterial;
}

//-----------------------------------------------------------------------------
// The shader shadow interface
//-----------------------------------------------------------------------------
CShaderShadowGL::CShaderShadowGL()
{
	m_IsTranslucent = false;
	m_IsAlphaTested = false;
	m_bIsDepthWriteEnabled = true;
	m_bDepthTestEnabled = true;
	m_SrcBlend = SHADER_BLEND_ONE;
	m_DstBlend = SHADER_BLEND_ZERO;
	m_bColorWriteEnabled = true;
	m_bAlphaWriteEnabled = true;
	m_bUsesVertexAndPixelShaders = false;
	m_VertexUsage = ComputeGLVertexFormat( VERTEX_POSITION | VERTEX_COLOR, 1, NULL, 0, 0 );
}

CShaderShadowGL::~CShaderShadowGL()
{
}

// Sets the default *shadow* state
void CShaderShadowGL::SetDefaultState()
{
	m_IsTranslucent = false;
	m_IsAlphaTested = false;
	m_bIsDepthWriteEnabled = true;
	m_bDepthTestEnabled = true;
	m_SrcBlend = SHADER_BLEND_ONE;
	m_DstBlend = SHADER_BLEND_ZERO;
	m_bColorWriteEnabled = true;
	m_bAlphaWriteEnabled = true;
	m_bUsesVertexAndPixelShaders = false;
	m_VertexUsage = ComputeGLVertexFormat( VERTEX_POSITION | VERTEX_COLOR, 1, NULL, 0, 0 );
}

// Methods related to depth buffering
void CShaderShadowGL::DepthFunc( ShaderDepthFunc_t depthFunc )
{
}

void CShaderShadowGL::EnableDepthWrites( bool bEnable )
{
	m_bIsDepthWriteEnabled = bEnable;
}

void CShaderShadowGL::EnableDepthTest( bool bEnable )
{
	m_bDepthTestEnabled = bEnable;
}

void CShaderShadowGL::EnablePolyOffset( PolygonOffsetMode_t nOffsetMode )
{
}

// Suppresses/activates color writing
void CShaderShadowGL::EnableColorWrites( bool bEnable )
{
	m_bColorWriteEnabled = bEnable;
}

// Suppresses/activates alpha writing
void CShaderShadowGL::EnableAlphaWrites( bool bEnable )
{
	m_bAlphaWriteEnabled = bEnable;
}

// Methods related to alpha blending
void CShaderShadowGL::EnableBlending( bool bEnable )
{
	m_IsTranslucent = bEnable;
}

void CShaderShadowGL::BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{
	m_SrcBlend = srcFactor;
	m_DstBlend = dstFactor;
}

// A simpler method of dealing with alpha modulation
void CShaderShadowGL::EnableAlphaPipe( bool bEnable )
{
}

void CShaderShadowGL::EnableConstantAlpha( bool bEnable )
{
}

void CShaderShadowGL::EnableVertexAlpha( bool bEnable )
{
}

void CShaderShadowGL::EnableTextureAlpha( TextureStage_t stage, bool bEnable )
{
}


// Alpha testing
void CShaderShadowGL::EnableAlphaTest( bool bEnable )
{
	m_IsAlphaTested = bEnable;
}

void CShaderShadowGL::AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ )
{
}


// Wireframe/filled polygons
void CShaderShadowGL::PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode )
{
}


// Back face culling
void CShaderShadowGL::EnableCulling( bool bEnable )
{
}


// Alpha to coverage
void CShaderShadowGL::EnableAlphaToCoverage( bool bEnable )
{
}


// constant color + transparency
void CShaderShadowGL::EnableConstantColor( bool bEnable )
{
}

// Indicates the vertex format for use with a vertex shader
// The flags to pass in here come from the VertexFormatFlags_t enum
// If pTexCoordDimensions is *not* specified, we assume all coordinates
// are 2-dimensional
void CShaderShadowGL::VertexShaderVertexFormat( unsigned int nFlags,
												   int nTexCoordCount,
												   int* pTexCoordDimensions,
												   int nUserDataSize )
{
	nFlags &= ~VERTEX_BONE_INDEX;
	nFlags |= VERTEX_FORMAT_VERTEX_SHADER;
	m_VertexUsage = ComputeGLVertexFormat( nFlags, nTexCoordCount, pTexCoordDimensions, 0, nUserDataSize );
}

// Indicates we're going to light the model
void CShaderShadowGL::EnableLighting( bool bEnable )
{
}

void CShaderShadowGL::EnableSpecular( bool bEnable )
{
}

// Activate/deactivate skinning
void CShaderShadowGL::EnableVertexBlend( bool bEnable )
{
}

// per texture unit stuff
void CShaderShadowGL::OverbrightValue( TextureStage_t stage, float value )
{
}

void CShaderShadowGL::EnableTexture( Sampler_t stage, bool bEnable )
{
}

void CShaderShadowGL::EnableCustomPixelPipe( bool bEnable )
{
}

void CShaderShadowGL::CustomTextureStages( int stageCount )
{
}

void CShaderShadowGL::CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel,
	ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 )
{
}

void CShaderShadowGL::EnableTexGen( TextureStage_t stage, bool bEnable )
{
}

void CShaderShadowGL::TexGen( TextureStage_t stage, ShaderTexGenParam_t param )
{
}

// Sets the vertex and pixel shaders
void CShaderShadowGL::SetVertexShader( const char *pShaderName, int vshIndex )
{
	m_bUsesVertexAndPixelShaders = ( pShaderName != NULL );
}

void CShaderShadowGL::EnableBlendingSeparateAlpha( bool bEnable )
{
}
void CShaderShadowGL::SetPixelShader( const char *pShaderName, int pshIndex )
{
	m_bUsesVertexAndPixelShaders = ( pShaderName != NULL );
}

void CShaderShadowGL::BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{
}
// indicates what per-vertex data we're providing
void CShaderShadowGL::DrawFlags( unsigned int drawFlags )
{
}



//-----------------------------------------------------------------------------
//
// Shader API Empty
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------

CShaderAPIGL::CShaderAPIGL()  : m_Mesh( true )
{
	m_MatrixMode = MATERIAL_MODEL;
	m_CurrentColor.Init( 1.0f, 1.0f, 1.0f, 1.0f );
	m_CurrentSnapshot = 0;
	m_pMaterial = NULL;
	m_Viewport.m_nTopLeftX = 0;
	m_Viewport.m_nTopLeftY = 0;
	m_Viewport.m_nWidth = 1280;
	m_Viewport.m_nHeight = 720;
	m_Viewport.m_flMinZ = 0.0f;
	m_Viewport.m_flMaxZ = 1.0f;
	for ( int i = 0; i < 4; i++ )
	{
		m_pBoundVertexBuffer[i] = NULL;
		m_nBoundVertexOffset[i] = 0;
		m_nBoundFirstVertex[i] = 0;
		m_nBoundVertexCount[i] = 0;
		m_BoundVertexFormat[i] = 0;
	}
	m_pBoundIndexBuffer = NULL;
	m_nBoundIndexOffset = 0;
}

CShaderAPIGL::~CShaderAPIGL()
{
}

// MaterialSystem releases the context on the old thread before scheduling
// acquisition on the render worker (and reverses this for synchronous work).
void CShaderAPIGL::AcquireThreadOwnership()
{
#if defined( USE_SDL ) && defined( DX_TO_GL_ABSTRACTION )
	if ( !g_pLauncherMgr || !g_pLauncherMgr->MakeContextCurrent( g_pLauncherMgr->GetMainContext() ) )
	{
		Error( "[GL] Failed to acquire render context: %s\n", SDL_GetError() );
		return;
	}
	static int s_nAcquireSamples = 0;
	if ( s_nAcquireSamples++ < 8 )
		Msg( "[GL] Acquired render context on thread %lu\n", (unsigned long)SDL_ThreadID() );
#endif
}

void CShaderAPIGL::ReleaseThreadOwnership()
{
#if defined( USE_SDL ) && defined( DX_TO_GL_ABSTRACTION )
	if ( !SDL_GL_GetCurrentContext() )
		return;
	// Submit pending commands while this thread still owns the context.
	glFlush();
	if ( !g_pLauncherMgr || !g_pLauncherMgr->MakeContextCurrent( NULL ) )
	{
		Error( "[GL] Failed to release render context: %s\n", SDL_GetError() );
		return;
	}
#endif
}

static GLenum GLBlendFactor( ShaderBlendFactor_t factor )
{
	switch ( factor )
	{
	case SHADER_BLEND_ZERO: return GL_ZERO;
	case SHADER_BLEND_ONE: return GL_ONE;
	case SHADER_BLEND_DST_COLOR: return GL_DST_COLOR;
	case SHADER_BLEND_ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
	case SHADER_BLEND_SRC_ALPHA: return GL_SRC_ALPHA;
	case SHADER_BLEND_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
	case SHADER_BLEND_DST_ALPHA: return GL_DST_ALPHA;
	case SHADER_BLEND_ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
	case SHADER_BLEND_SRC_ALPHA_SATURATE: return GL_SRC_ALPHA_SATURATE;
	case SHADER_BLEND_SRC_COLOR: return GL_SRC_COLOR;
	case SHADER_BLEND_ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
	default: return GL_ONE;
	}
}

static void ApplyGLBlendState( bool enabled, ShaderBlendFactor_t src, ShaderBlendFactor_t dst )
{
	if ( enabled )
	{
		glEnable( GL_BLEND );
		glBlendEquation( GL_FUNC_ADD );
		glBlendFunc( GLBlendFactor( src ), GLBlendFactor( dst ) );
	}
	else glDisable( GL_BLEND );
}

void CShaderAPIGL::ApplyMaterialWriteMasks( IMaterial *pMaterial, bool bAlphaModulation ) const
{
	if ( !pMaterial ) return;
	ShaderRenderState_t *pState = static_cast<IMaterialInternal*>( pMaterial )->GetRenderState();
	if ( !pState || !pState->m_pSnapshots ) return;
	// Opaque materials have a separate blend/depth state when AlphaModulate fades them.
	int modulation = GetGLAlphaSnapshotIndex( pState, bAlphaModulation );
	if ( pState->m_pSnapshots[modulation].m_nPassCount <= 0 ) return;
	int id = (int)pState->m_pSnapshots[modulation].m_Snapshot[0];
	if ( id < 0 || id >= m_Snapshots.Count() ) return;
	const SnapshotRecord_t &rec = m_Snapshots[id];
	bool blend = ( rec.m_Flags & TRANSLUCENT ) != 0;
	ApplyGLBlendState( blend, rec.m_SrcBlend, rec.m_DstBlend );
	glUniform1i( s_uTranslucent, blend ? 1 : 0 );
	if ( !rec.m_bDepthTest ) glDisable( GL_DEPTH_TEST );
	glColorMask( rec.m_bColorWrite, rec.m_bColorWrite, rec.m_bColorWrite, rec.m_bAlphaWrite );
	if ( !( rec.m_Flags & DEPTHWRITE ) ) glDepthMask( GL_FALSE );
}

void CShaderAPIGL::BindVertexBuffer( int streamID, IVertexBuffer *pVertexBuffer, int nOffsetInBytes, int nFirstVertex, int nVertexCount, VertexFormat_t fmt, int nRepetitions1 )
{
	if ( streamID >= 0 && streamID < 4 )
	{
		m_pBoundVertexBuffer[streamID] = pVertexBuffer;
		m_nBoundVertexOffset[streamID] = nOffsetInBytes;
		m_nBoundFirstVertex[streamID] = nFirstVertex;
		m_nBoundVertexCount[streamID] = nVertexCount;
		m_BoundVertexFormat[streamID] = fmt;
	}
}

void CShaderAPIGL::BindIndexBuffer( IIndexBuffer *pIndexBuffer, int nOffsetInBytes )
{
	m_pBoundIndexBuffer = pIndexBuffer;
	m_nBoundIndexOffset = nOffsetInBytes;
}

void CShaderAPIGL::Draw( MaterialPrimitiveType_t primitiveType, int firstIndex, int numIndices )
{
	CMeshGL *pVB = m_pBoundVertexBuffer[0] ? static_cast<CMeshGL*>( m_pBoundVertexBuffer[0] ) : ( m_Mesh.m_pVertexOverride ? m_Mesh.m_pVertexOverride : &m_Mesh );
	if ( m_BoundVertexFormat[0] != 0 && pVB )
	{
		pVB->SetVertexFormat( m_BoundVertexFormat[0] );
	}
	CMeshGL *pIB = m_pBoundIndexBuffer ? static_cast<CMeshGL*>( m_pBoundIndexBuffer ) : ( m_Mesh.m_pIndexOverride ? m_Mesh.m_pIndexOverride : pVB );

	int actualFirstIndex = firstIndex + ( m_nBoundIndexOffset / (int)sizeof(unsigned short) );
	DrawMeshInternal( pVB, pIB, primitiveType, actualFirstIndex, numIndices, m_pMaterial, m_pBoundVertexBuffer[0] ? m_nBoundVertexOffset[0] : 0 );
}


bool CShaderAPIGL::DoRenderTargetsNeedSeparateDepthBuffer() const
{
	return false;
}

// Can we download textures?
bool CShaderAPIGL::CanDownloadTextures() const
{
	return true;
}

// Used to clear the transition table when we know it's become invalid.
void CShaderAPIGL::ClearSnapshots()
{
	m_Snapshots.RemoveAll();
}

// Members of IMaterialSystemHardwareConfig
bool CShaderAPIGL::HasDestAlphaBuffer() const
{
	return false;
}

bool CShaderAPIGL::HasStencilBuffer() const
{
	return false;
}

int CShaderAPIGL::MaxViewports() const
{
	return 1;
}

int CShaderAPIGL::GetShadowFilterMode() const
{
	return 0;
}

int CShaderAPIGL::StencilBufferBits() const
{
	return 0;
}

int	 CShaderAPIGL::GetFrameBufferColorDepth() const
{
	return 0;
}

int  CShaderAPIGL::GetSamplerCount() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 60))
		return 1;
	if (( ShaderUtil()->GetConfig().dxSupportLevel >= 60 ) && ( ShaderUtil()->GetConfig().dxSupportLevel < 80 ))
		return 2;
	return 4;
}

bool CShaderAPIGL::HasSetDeviceGammaRamp() const
{
	return false;
}

bool CShaderAPIGL::SupportsCompressedTextures() const
{
	return false;
}

VertexCompressionType_t CShaderAPIGL::SupportsCompressedVertices() const
{
	return VERTEX_COMPRESSION_NONE;
}

bool CShaderAPIGL::SupportsVertexAndPixelShaders() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 80))
		return false;

	return true;
}

bool CShaderAPIGL::SupportsPixelShaders_1_4() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 81))
		return false;

	return true;
}

bool CShaderAPIGL::SupportsPixelShaders_2_0() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 90))
		return false;

	return true;
}

bool CShaderAPIGL::SupportsPixelShaders_2_b() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 90))
		return false;

	return true;
}

bool CShaderAPIGL::ActuallySupportsPixelShaders_2_b() const
{
	return true;
}

bool CShaderAPIGL::SupportsShaderModel_3_0() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
		(ShaderUtil()->GetConfig().dxSupportLevel < 95))
		return false;

	return true;
}

bool CShaderAPIGL::SupportsStaticControlFlow() const
{
	if ( IsOpenGL() )
		return false;

	return SupportsVertexShaders_2_0();
}

bool CShaderAPIGL::SupportsVertexShaders_2_0() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 90))
		return false;

	return true;
}

int  CShaderAPIGL::MaximumAnisotropicLevel() const
{
	return 0;
}

void CShaderAPIGL::SetAnisotropicLevel( int nAnisotropyLevel )
{
}

int  CShaderAPIGL::MaxTextureWidth() const
{
	// Should be big enough to cover all cases
	return 16384;
}

int  CShaderAPIGL::MaxTextureHeight() const
{
	// Should be big enough to cover all cases
	return 16384;
}

int  CShaderAPIGL::MaxTextureAspectRatio() const
{
	// Should be big enough to cover all cases
	return 16384;
}


int	 CShaderAPIGL::TextureMemorySize() const
{
	// fake it
	return 64 * 1024 * 1024;
}

int  CShaderAPIGL::GetDXSupportLevel() const
{
	return 90;
}

bool CShaderAPIGL::SupportsOverbright() const
{
	return false;
}

bool CShaderAPIGL::SupportsCubeMaps() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
		return false;

	return true;
}

bool CShaderAPIGL::SupportsNonPow2Textures() const
{
	return true;
}

bool CShaderAPIGL::SupportsMipmappedCubemaps() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
		return false;

	return true;
}

int  CShaderAPIGL::GetTextureStageCount() const
{
	return 4;
}

int	 CShaderAPIGL::NumVertexShaderConstants() const
{
	return 128;
}

int	 CShaderAPIGL::NumBooleanVertexShaderConstants() const
{
	return 0;
}

int	 CShaderAPIGL::NumIntegerVertexShaderConstants() const
{
	return 0;
}

int	 CShaderAPIGL::NumPixelShaderConstants() const
{
	return 8;
}

int	 CShaderAPIGL::MaxNumLights() const
{
	return 4;
}

bool CShaderAPIGL::SupportsSpheremapping() const
{
	return false;
}


// This is the max dx support level supported by the card
int	CShaderAPIGL::GetMaxDXSupportLevel() const
{
	return 90;
}

bool CShaderAPIGL::SupportsHardwareLighting() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
		return false;

	return true;
}

int	 CShaderAPIGL::MaxBlendMatrices() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
	{
		return 1;
	}

	return 0;
}

int	 CShaderAPIGL::MaxBlendMatrixIndices() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
	{
		return 1;
	}

	return 0;
}

int	 CShaderAPIGL::MaxVertexShaderBlendMatrices() const
{
	return 0;
}

int	CShaderAPIGL::MaxUserClipPlanes() const
{
	return 0;
}

bool CShaderAPIGL::SpecifiesFogColorInLinearSpace() const
{
	return false;
}

bool CShaderAPIGL::SupportsSRGB() const
{
	return false;
}

bool CShaderAPIGL::FakeSRGBWrite() const
{
	return false;
}

bool CShaderAPIGL::CanDoSRGBReadFromRTs() const
{
	return true;
}

bool CShaderAPIGL::SupportsGLMixedSizeTargets() const
{
	return false;
}

const char *CShaderAPIGL::GetHWSpecificShaderDLLName() const
{
	return 0;
}

// Sets the default *dynamic* state
void CShaderAPIGL::SetDefaultState()
{
}


// Returns the snapshot id for the shader state
StateSnapshot_t	 CShaderAPIGL::TakeSnapshot( )
{
	SnapshotRecord_t rec;
	rec.m_bDepthTest = g_ShaderShadowGL.m_bDepthTestEnabled;
	rec.m_SrcBlend = g_ShaderShadowGL.m_SrcBlend;
	rec.m_DstBlend = g_ShaderShadowGL.m_DstBlend;
	rec.m_bColorWrite = g_ShaderShadowGL.m_bColorWriteEnabled;
	rec.m_bAlphaWrite = g_ShaderShadowGL.m_bAlphaWriteEnabled;
	rec.m_Flags = 0;
	if ( g_ShaderShadowGL.m_IsTranslucent )
		rec.m_Flags |= TRANSLUCENT;
	if ( g_ShaderShadowGL.m_IsAlphaTested )
		rec.m_Flags |= ALPHATESTED;
	if ( g_ShaderShadowGL.m_bUsesVertexAndPixelShaders )
		rec.m_Flags |= VERTEX_AND_PIXEL_SHADERS;
	if ( g_ShaderShadowGL.m_bIsDepthWriteEnabled )
		rec.m_Flags |= DEPTHWRITE;

	rec.m_VertexUsage = g_ShaderShadowGL.m_VertexUsage;
	if ( rec.m_VertexUsage == 0 )
	{
		rec.m_VertexUsage = ComputeGLVertexFormat( VERTEX_POSITION | VERTEX_COLOR, 1, NULL, 0, 0 );
	}

	int id = m_Snapshots.AddToTail( rec );
	return (StateSnapshot_t)id;
}

// Returns true if the state snapshot is transparent
bool CShaderAPIGL::IsTranslucent( StateSnapshot_t id ) const
{
	int idx = (int)id;
	if ( idx >= 0 && idx < m_Snapshots.Count() )
		return ( m_Snapshots[idx].m_Flags & TRANSLUCENT ) != 0;
	return false;
}

bool CShaderAPIGL::IsAlphaTested( StateSnapshot_t id ) const
{
	int idx = (int)id;
	if ( idx >= 0 && idx < m_Snapshots.Count() )
		return ( m_Snapshots[idx].m_Flags & ALPHATESTED ) != 0;
	return false;
}

bool CShaderAPIGL::IsDepthWriteEnabled( StateSnapshot_t id ) const
{
	int idx = (int)id;
	if ( idx >= 0 && idx < m_Snapshots.Count() )
		return ( m_Snapshots[idx].m_Flags & DEPTHWRITE ) != 0;
	return true;
}

bool CShaderAPIGL::UsesVertexAndPixelShaders( StateSnapshot_t id ) const
{
	int idx = (int)id;
	if ( idx >= 0 && idx < m_Snapshots.Count() )
		return ( m_Snapshots[idx].m_Flags & VERTEX_AND_PIXEL_SHADERS ) != 0;
	return false;
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPIGL::ComputeVertexFormat( int numSnapshots, StateSnapshot_t* pIds ) const
{
	return ComputeVertexUsage( numSnapshots, pIds );
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPIGL::ComputeVertexUsage( int numSnapshots, StateSnapshot_t* pIds ) const
{
	if ( numSnapshots <= 0 || !pIds )
		return ComputeGLVertexFormat( VERTEX_POSITION | VERTEX_COLOR, 1, NULL, 0, 0 );

	VertexFormat_t fmt = 0;
	for ( int i = 0; i < numSnapshots; ++i )
	{
		int id = (int)pIds[i];
		if ( id >= 0 && id < m_Snapshots.Count() )
		{
			fmt |= m_Snapshots[id].m_VertexUsage;
		}
	}
	if ( fmt == 0 )
	{
		fmt = ComputeGLVertexFormat( VERTEX_POSITION | VERTEX_COLOR, 1, NULL, 0, 0 );
	}
	return fmt;
}

// Uses a state snapshot
void CShaderAPIGL::UseSnapshot( StateSnapshot_t snapshot )
{
	m_CurrentSnapshot = snapshot;
}

// Sets the color to modulate by
void CShaderAPIGL::Color3f( float r, float g, float b )
{
	m_CurrentColor.Init( r, g, b, 1.0f );
}

void CShaderAPIGL::Color3fv( float const* pColor )
{
	if ( pColor )
		m_CurrentColor.Init( pColor[0], pColor[1], pColor[2], 1.0f );
}

void CShaderAPIGL::Color4f( float r, float g, float b, float a )
{
	m_CurrentColor.Init( r, g, b, a );
}

void CShaderAPIGL::Color4fv( float const* pColor )
{
	if ( pColor )
		m_CurrentColor.Init( pColor[0], pColor[1], pColor[2], pColor[3] );
}

// Faster versions of color
void CShaderAPIGL::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
	m_CurrentColor.Init( (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f );
}

void CShaderAPIGL::Color3ubv( unsigned char const* rgb )
{
	if ( rgb )
		m_CurrentColor.Init( (float)rgb[0] / 255.0f, (float)rgb[1] / 255.0f, (float)rgb[2] / 255.0f, 1.0f );
}

void CShaderAPIGL::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	m_CurrentColor.Init( (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f );
}

void CShaderAPIGL::Color4ubv( unsigned char const* rgba )
{
	if ( rgba )
		m_CurrentColor.Init( (float)rgba[0] / 255.0f, (float)rgba[1] / 255.0f, (float)rgba[2] / 255.0f, (float)rgba[3] / 255.0f );
}

// The shade mode
void CShaderAPIGL::ShadeMode( ShaderShadeMode_t mode )
{
}

// Binds a particular material to render with
void CShaderAPIGL::Bind( IMaterial* pMaterial )
{
	m_pMaterial = pMaterial;
	BindGLMaterialTexture( pMaterial );
}

// Cull mode
void CShaderAPIGL::CullMode( MaterialCullMode_t cullMode )
{
}

void CShaderAPIGL::ForceDepthFuncEquals( bool bEnable )
{
}

// Forces Z buffering on or off
void CShaderAPIGL::OverrideDepthEnable( bool bEnable, bool bDepthEnable )
{
}

void CShaderAPIGL::OverrideAlphaWriteEnable( bool bOverrideEnable, bool bAlphaWriteEnable )
{
}

void CShaderAPIGL::OverrideColorWriteEnable( bool bOverrideEnable, bool bColorWriteEnable )
{
}

//legacy fast clipping linkage
void CShaderAPIGL::SetHeightClipZ( float z )
{
}

void CShaderAPIGL::SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode )
{
}

// Sets the lights
void CShaderAPIGL::SetLight( int lightNum, const LightDesc_t& desc )
{
}

// Sets lighting origin for the current model
void CShaderAPIGL::SetLightingOrigin( Vector vLightingOrigin )
{
}

void CShaderAPIGL::SetAmbientLight( float r, float g, float b )
{
}

void CShaderAPIGL::SetAmbientLightCube( Vector4D cube[6] )
{
}

// Get lights
int CShaderAPIGL::GetMaxLights( void ) const
{
	return 0;
}

const LightDesc_t& CShaderAPIGL::GetLight( int lightNum ) const
{
	static LightDesc_t blah;
	return blah;
}

// Render state for the ambient light cube (vertex shaders)
void CShaderAPIGL::SetVertexShaderStateAmbientLightCube()
{
}

void CShaderAPIGL::SetSkinningMatrices()
{
}

// Lightmap texture binding
void CShaderAPIGL::BindLightmap( TextureStage_t stage )
{
	g_pShaderUtil->BindStandardTexture( (Sampler_t)stage, TEXTURE_LIGHTMAP );
}

void CShaderAPIGL::BindBumpLightmap( TextureStage_t stage )
{
	g_pShaderUtil->BindStandardTexture( (Sampler_t)stage, TEXTURE_LIGHTMAP_BUMPED );
}

void CShaderAPIGL::BindFullbrightLightmap( TextureStage_t stage )
{
	g_pShaderUtil->BindStandardTexture( (Sampler_t)stage, TEXTURE_LIGHTMAP_FULLBRIGHT );
}

void CShaderAPIGL::BindWhite( TextureStage_t stage )
{
	BindTexture( (Sampler_t)stage, (ShaderAPITextureHandle_t)GetWhiteTexture() );
}

void CShaderAPIGL::BindBlack( TextureStage_t stage )
{
	BindTexture( (Sampler_t)stage, (ShaderAPITextureHandle_t)GetBlackTexture() );
}

void CShaderAPIGL::BindGrey( TextureStage_t stage )
{
	BindTexture( (Sampler_t)stage, (ShaderAPITextureHandle_t)GetGreyTexture() );
}

// Gets the lightmap dimensions
void CShaderAPIGL::GetLightmapDimensions( int *w, int *h )
{
	g_pShaderUtil->GetLightmapDimensions( w, h );
}

// Special system flat normal map binding.
void CShaderAPIGL::BindFlatNormalMap( TextureStage_t stage )
{
}

void CShaderAPIGL::BindNormalizationCubeMap( TextureStage_t stage )
{
}

void CShaderAPIGL::BindSignedNormalizationCubeMap( TextureStage_t stage )
{
}

void CShaderAPIGL::BindFBTexture( TextureStage_t stage, int textureIndex )
{
}

// Flushes any primitives that are buffered
void CShaderAPIGL::FlushBufferedPrimitives()
{
}

// Gets the dynamic mesh; note that you've got to render the mesh
// before calling this function a second time. Clients should *not*
// call DestroyStaticMesh on the mesh returned by this call.
IMesh* CShaderAPIGL::GetDynamicMesh( IMaterial* pMaterial, int nHWSkinBoneCount, bool buffered, IMesh* pVertexOverride, IMesh* pIndexOverride )
{
	VertexFormat_t fmt = pVertexOverride ? pVertexOverride->GetVertexFormat() :
		( pMaterial ? pMaterial->GetVertexFormat() : ComputeGLVertexFormat( VERTEX_POSITION | VERTEX_COLOR, 1, NULL, 0, 0 ) );
	return GetDynamicMeshEx( pMaterial, fmt, nHWSkinBoneCount, buffered, pVertexOverride, pIndexOverride );
}

IMesh* CShaderAPIGL::GetDynamicMeshEx( IMaterial* pMaterial, VertexFormat_t fmt, int nHWSkinBoneCount, bool buffered, IMesh* pVertexOverride, IMesh* pIndexOverride )
{
	m_pMaterial = pMaterial;
	m_Mesh.SetMaterial( pMaterial );
	m_Mesh.SetVertexFormat( pVertexOverride ? pVertexOverride->GetVertexFormat() : ( fmt & ~VERTEX_FORMAT_COMPRESSED ) );
	m_Mesh.SetVertexOverride( static_cast<CMeshGL*>( pVertexOverride ) );
	m_Mesh.SetIndexOverride( static_cast<CMeshGL*>( pIndexOverride ) );
	if ( pMaterial )
	{
		Bind( pMaterial );
	}
	return &m_Mesh;
}

IMesh* CShaderAPIGL::GetFlexMesh()
{
	return &m_Mesh;
}

// Begins a rendering pass that uses a state snapshot
void CShaderAPIGL::BeginPass( StateSnapshot_t snapshot  )
{
}

// Renders a single pass of a material
void CShaderAPIGL::RenderPass( int nPass, int nPassCount )
{
}

// stuff related to matrix stacks
void CShaderAPIGL::MatrixMode( MaterialMatrixMode_t matrixMode )
{
	if ( matrixMode >= 0 && matrixMode <= MATERIAL_MODEL_MAX )
		m_MatrixMode = matrixMode;
}

void CShaderAPIGL::PushMatrix()
{
	if ( m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
		m_MatrixStack[m_MatrixMode].Push();
}

void CShaderAPIGL::PopMatrix()
{
	if ( m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
		m_MatrixStack[m_MatrixMode].Pop();
}

void CShaderAPIGL::LoadMatrix( float *m )
{
	if ( m && m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
	{
		m_MatrixStack[m_MatrixMode].LoadMatrix( m );
	}
}

void CShaderAPIGL::MultMatrix( float *m )
{
	if ( m && m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
	{
		m_MatrixStack[m_MatrixMode].MultMatrix( m );
	}
}

void CShaderAPIGL::MultMatrixLocal( float *m )
{
	if ( m && m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
	{
		m_MatrixStack[m_MatrixMode].MultMatrixLocal( m );
	}
}

void CShaderAPIGL::GetMatrix( MaterialMatrixMode_t matrixMode, float *dst )
{
	if ( dst && matrixMode >= 0 && matrixMode <= MATERIAL_MODEL_MAX )
	{
		memcpy( dst, m_MatrixStack[matrixMode].Top().Base(), 16 * sizeof(float) );
	}
}

void CShaderAPIGL::LoadIdentity( void )
{
	if ( m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
	{
		m_MatrixStack[m_MatrixMode].LoadIdentity();
	}
}

void CShaderAPIGL::LoadCameraToWorld( void )
{
	if ( m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
	{
		VMatrix camToWorld;
		if ( MatrixInverseGeneral( m_MatrixStack[MATERIAL_VIEW].Top(), camToWorld ) )
		{
			m_MatrixStack[m_MatrixMode].LoadMatrix( camToWorld.Base() );
		}
	}
}

void CShaderAPIGL::Ortho( double left, double top, double right, double bottom, double zNear, double zFar )
{
	if ( m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
	{
		MatrixOrtho( m_MatrixStack[m_MatrixMode].Top(), left, top, right, bottom, zNear, zFar );
	}
}

void CShaderAPIGL::PerspectiveX( double fovx, double aspect, double zNear, double zFar )
{
	if ( m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
	{
		MatrixPerspectiveX( m_MatrixStack[m_MatrixMode].Top(), fovx, aspect, zNear, zFar );
	}
}

void CShaderAPIGL::PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right )
{
	if ( m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
	{
		MatrixPerspectiveOffCenterX( m_MatrixStack[m_MatrixMode].Top(), fovx, aspect, zNear, zFar, bottom, top, left, right );
	}
}

void CShaderAPIGL::PickMatrix( int x, int y, int width, int height )
{
}

void CShaderAPIGL::Rotate( float angle, float x, float y, float z )
{
	if ( m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
	{
		Vector axis( x, y, z );
		m_MatrixStack[m_MatrixMode].RotateAxisLocal( axis, angle );
	}
}

void CShaderAPIGL::Translate( float x, float y, float z )
{
	if ( m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
	{
		m_MatrixStack[m_MatrixMode].TranslateLocal( x, y, z );
	}
}

void CShaderAPIGL::Scale( float x, float y, float z )
{
	if ( m_MatrixMode >= 0 && m_MatrixMode <= MATERIAL_MODEL_MAX )
	{
		m_MatrixStack[m_MatrixMode].ScaleLocal( x, y, z );
	}
}

void CShaderAPIGL::ScaleXY( float x, float y )
{
	Scale( x, y, 1.0f );
}

// Fog methods...
void CShaderAPIGL::FogMode( MaterialFogMode_t fogMode )
{
}

void CShaderAPIGL::FogStart( float fStart )
{
}

void CShaderAPIGL::FogEnd( float fEnd )
{
}

void CShaderAPIGL::SetFogZ( float fogZ )
{
}

void CShaderAPIGL::FogMaxDensity( float flMaxDensity )
{
}

void CShaderAPIGL::GetFogDistances( float *fStart, float *fEnd, float *fFogZ )
{
}


void CShaderAPIGL::SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}


void CShaderAPIGL::SceneFogMode( MaterialFogMode_t fogMode )
{
}

void CShaderAPIGL::GetSceneFogColor( unsigned char *rgb )
{
}

MaterialFogMode_t CShaderAPIGL::GetSceneFogMode( )
{
	return MATERIAL_FOG_NONE;
}

int CShaderAPIGL::GetPixelFogCombo( )
{
	return 0;
}

void CShaderAPIGL::FogColor3f( float r, float g, float b )
{
}

void CShaderAPIGL::FogColor3fv( float const* rgb )
{
}

void CShaderAPIGL::FogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPIGL::FogColor3ubv( unsigned char const* rgb )
{
}

void CShaderAPIGL::SetViewports( int nCount, const ShaderViewport_t* pViewports )
{
	if ( nCount > 0 && pViewports )
	{
		m_Viewport = pViewports[0];
		int targetWidth, targetHeight;
		GetGLTargetSize( targetWidth, targetHeight );
		glViewport( pViewports[0].m_nTopLeftX, targetHeight - pViewports[0].m_nTopLeftY - pViewports[0].m_nHeight, pViewports[0].m_nWidth, pViewports[0].m_nHeight );
		float minZ = pViewports[0].m_flMinZ;
		float maxZ = pViewports[0].m_flMaxZ;
		if ( minZ == 0.0f && maxZ == 0.0f )
		{
			maxZ = 1.0f;
		}
		glDepthRangef( minZ, maxZ );
	}
}

int CShaderAPIGL::GetViewports( ShaderViewport_t* pViewports, int nMax ) const
{
	if ( nMax > 0 && pViewports )
	{
		pViewports[0] = m_Viewport;
		return 1;
	}
	return 0;
}

// Sets the vertex and pixel shaders
void CShaderAPIGL::SetVertexShaderIndex( int vshIndex )
{
}

void CShaderAPIGL::SetPixelShaderIndex( int pshIndex )
{
}

// Sets the constant registers for vertex and pixel shaders
void CShaderAPIGL::SetVertexShaderConstant( int var, float const* pVec, int numConst, bool bForce )
{
}

void CShaderAPIGL::SetBooleanVertexShaderConstant( int var, BOOL const* pVec, int numConst, bool bForce )
{
}

void CShaderAPIGL::SetIntegerVertexShaderConstant( int var, int const* pVec, int numConst, bool bForce )
{
}

void CShaderAPIGL::SetPixelShaderConstant( int var, float const* pVec, int numConst, bool bForce )
{
}

void CShaderAPIGL::SetBooleanPixelShaderConstant( int var, BOOL const* pVec, int numBools, bool bForce )
{
}

void CShaderAPIGL::SetIntegerPixelShaderConstant( int var, int const* pVec, int numIntVecs, bool bForce )
{
}

void CShaderAPIGL::InvalidateDelayedShaderConstants( void )
{
}

float CShaderAPIGL::GammaToLinear_HardwareSpecific( float fGamma ) const
{
	return 0.0f;
}

float CShaderAPIGL::LinearToGamma_HardwareSpecific( float fLinear ) const
{
	return 0.0f;
}

void CShaderAPIGL::SetLinearToGammaConversionTextures( ShaderAPITextureHandle_t hSRGBWriteEnabledTexture, ShaderAPITextureHandle_t hIdentityTexture )
{
}


// Returns the nearest supported format
struct GLTextureRecord_t
{
	GLuint texture;
	int width, height;
	GLuint framebuffer, depthbuffer;
	unsigned char *lockShadow;
};
static CUtlVector<GLTextureRecord_t> s_GLTextures;
static GLuint s_ModifyTexture = 0;
static CUtlMemory<unsigned char> s_TextureLockBits;
static GLuint s_LockedTexture = 0;
static int s_LockLevel, s_LockX, s_LockY, s_LockWidth, s_LockHeight;

static GLTextureRecord_t *FindGLTexture( GLuint texture )
{
	for ( int i = 0; i < s_GLTextures.Count(); ++i )
		if ( s_GLTextures[i].texture == texture ) return &s_GLTextures[i];
	return NULL;
}

static void GetGLTargetSize( int &width, int &height )
{
	GLint framebuffer;
	glGetIntegerv( GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer );
	for ( int i = 0; framebuffer && i < s_GLTextures.Count(); ++i )
	{
		if ( s_GLTextures[i].framebuffer == (GLuint)framebuffer )
		{
			width = s_GLTextures[i].width;
			height = s_GLTextures[i].height;
			return;
		}
	}
	g_ShaderDeviceGL.GetBackBufferDimensions( width, height );
}

static GLuint GetGLFramebuffer( GLTextureRecord_t *pTexture )
{
	if ( !pTexture->framebuffer )
	{
		GLint previous;
		glGetIntegerv( GL_FRAMEBUFFER_BINDING, &previous );
		glGenFramebuffers( 1, &pTexture->framebuffer );
		glBindFramebuffer( GL_FRAMEBUFFER, pTexture->framebuffer );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pTexture->texture, 0 );
		glGenRenderbuffers( 1, &pTexture->depthbuffer );
		glBindRenderbuffer( GL_RENDERBUFFER, pTexture->depthbuffer );
		glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, pTexture->width, pTexture->height );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, pTexture->depthbuffer );
		GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
		if ( status != GL_FRAMEBUFFER_COMPLETE )
			Error( "[GL] Incomplete render target %u: 0x%x\n", pTexture->texture, status );
		glBindFramebuffer( GL_FRAMEBUFFER, previous );
	}
	return pTexture->framebuffer;
}

void CShaderAPIGL::SetRenderTarget( ShaderAPITextureHandle_t color, ShaderAPITextureHandle_t depth )
{
	SetRenderTargetEx( 0, color, depth );
}

void CShaderAPIGL::SetRenderTargetEx( int id, ShaderAPITextureHandle_t color, ShaderAPITextureHandle_t depth )
{
	if ( id != 0 ) return;
	if ( color == (ShaderAPITextureHandle_t)SHADER_RENDERTARGET_BACKBUFFER )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		return;
	}
	GLTextureRecord_t *pTexture = FindGLTexture( (GLuint)color );
	if ( !pTexture )
	{
		Warning( "[GL] Unknown render target %u\n", (unsigned int)color );
		return;
	}
	glBindFramebuffer( GL_FRAMEBUFFER, GetGLFramebuffer( pTexture ) );
}

void CShaderAPIGL::CopyRenderTargetToTexture( ShaderAPITextureHandle_t texture )
{
	CopyRenderTargetToTextureEx( texture, 0, NULL, NULL );
}

void CShaderAPIGL::CopyRenderTargetToTextureEx( ShaderAPITextureHandle_t texture, int id, Rect_t *src, Rect_t *dst )
{
	GLTextureRecord_t *pTexture = FindGLTexture( (GLuint)texture );
	if ( id != 0 || !pTexture ) return;
	GLint read, draw;
	glGetIntegerv( GL_READ_FRAMEBUFFER_BINDING, &read );
	glGetIntegerv( GL_DRAW_FRAMEBUFFER_BINDING, &draw );
	int sourceWidth, sourceHeight;
	GetGLTargetSize( sourceWidth, sourceHeight );
	GLuint target = GetGLFramebuffer( pTexture );
	if ( target == (GLuint)draw ) return;
	int x = src ? src->x : m_Viewport.m_nTopLeftX;
	int y = src ? src->y : m_Viewport.m_nTopLeftY;
	int w = src ? src->width : m_Viewport.m_nWidth;
	int h = src ? src->height : m_Viewport.m_nHeight;
	int dx = dst ? dst->x : 0, dy = dst ? dst->y : 0;
	int dw = dst ? dst->width : pTexture->width, dh = dst ? dst->height : pTexture->height;
	glBindFramebuffer( GL_READ_FRAMEBUFFER, draw );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, target );
	GLboolean scissor = glIsEnabled( GL_SCISSOR_TEST );
	glDisable( GL_SCISSOR_TEST );
	// API rectangles start at the top; GLES framebuffer coordinates start at the bottom.
	int sourceY = sourceHeight - y - h;
	int destY = pTexture->height - dy - dh;
	glBlitFramebuffer( x, sourceY, x+w, sourceY+h, dx, destY, dx+dw, destY+dh, GL_COLOR_BUFFER_BIT, GL_LINEAR );
	if ( scissor ) glEnable( GL_SCISSOR_TEST );
	glBindFramebuffer( GL_READ_FRAMEBUFFER, read );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, draw );
}

ImageFormat CShaderAPIGL::GetNearestSupportedFormat( ImageFormat fmt, bool bFilteringRequired /* = true */ ) const
{
	return fmt;
}

ImageFormat CShaderAPIGL::GetNearestRenderTargetFormat( ImageFormat fmt ) const
{
	return fmt;
}

// Sets the texture state
void CShaderAPIGL::BindTexture( Sampler_t stage, ShaderAPITextureHandle_t textureHandle )
{
	glActiveTexture( GL_TEXTURE0 + (int)stage );
	GLuint tex = (GLuint)textureHandle;
	if ( !tex )
	{
		tex = GetWhiteTexture();
	}
	glBindTexture( GL_TEXTURE_2D, tex );
}

void CShaderAPIGL::ClearColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
	glClearColor( (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f );
}

void CShaderAPIGL::ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	glClearColor( (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f );
}

// Indicates we're going to be modifying this texture
// TexImage2D, TexSubImage2D, TexWrap, TexMinFilter, and TexMagFilter
// all use the texture specified by this function.
void CShaderAPIGL::ModifyTexture( ShaderAPITextureHandle_t textureHandle )
{
	s_ModifyTexture = (GLuint)textureHandle;
	if ( textureHandle )
	{
		glBindTexture( GL_TEXTURE_2D, (GLuint)textureHandle );
	}
}

// Texture management methods
void CShaderAPIGL::TexImage2D( int level, int cubeFace, ImageFormat dstFormat, int zOffset, int width, int height,
						 ImageFormat srcFormat, bool bSrcIsTiled, void *imageData )
{
	if ( !imageData || width <= 0 || height <= 0 )
		return;

	if ( srcFormat == IMAGE_FORMAT_RGBA8888 )
	{
		glTexImage2D( GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData );
	}
	else
	{
		int size = width * height * 4;
		CUtlMemory<unsigned char> tempBuf;
		tempBuf.EnsureCapacity( size );
		if ( ImageLoader::ConvertImageFormat( (const unsigned char*)imageData, srcFormat, (unsigned char*)tempBuf.Base(), IMAGE_FORMAT_RGBA8888, width, height ) )
		{
			glTexImage2D( GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tempBuf.Base() );
		}
		else
		{
			Warning( "[GL] Unsupported texture upload format %d\n", (int)srcFormat );
		}
	}
}

void CShaderAPIGL::TexSubImage2D( int level, int cubeFace, int xOffset, int yOffset, int zOffset, int width, int height,
						 ImageFormat srcFormat, int srcStride, bool bSrcIsTiled, void *imageData )
{
	if ( !imageData || width <= 0 || height <= 0 )
		return;

	if ( srcFormat == IMAGE_FORMAT_RGBA8888 && (srcStride == 0 || srcStride == width * 4) )
	{
		glTexSubImage2D( GL_TEXTURE_2D, level, xOffset, yOffset, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageData );
	}
	else
	{
		int size = width * height * 4;
		CUtlMemory<unsigned char> tempBuf;
		tempBuf.EnsureCapacity( size );
		if ( ImageLoader::ConvertImageFormat( (const unsigned char*)imageData, srcFormat, (unsigned char*)tempBuf.Base(), IMAGE_FORMAT_RGBA8888, width, height, srcStride, width * 4 ) )
		{
			glTexSubImage2D( GL_TEXTURE_2D, level, xOffset, yOffset, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tempBuf.Base() );
		}
		else
		{
			Warning( "[GL] Unsupported texture update format %d\n", (int)srcFormat );
		}
	}
}

void CShaderAPIGL::TexImageFromVTF( IVTFTexture *pVTF, int iVTFFrame )
{
	if ( !pVTF )
		return;

	int mipCount = pVTF->MipCount();
	ImageFormat fmt = pVTF->Format();

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, ( mipCount > 0 ) ? ( mipCount - 1 ) : 0 );

	for ( int iMip = 0; iMip < mipCount; ++iMip )
	{
		int w, h, d;
		pVTF->ComputeMipLevelDimensions( iMip, &w, &h, &d );
		unsigned char *pData = pVTF->ImageData( iVTFFrame, 0, iMip );
		if ( pData && w > 0 && h > 0 )
		{
			TexImage2D( iMip, 0, fmt, 0, w, h, fmt, false, pData );
		}
	}
}

bool CShaderAPIGL::TexLock( int level, int cubeFaceID, int xOffset, int yOffset,
								int width, int height, CPixelWriter& writer )
{
	GLTextureRecord_t *pTexture = FindGLTexture( s_ModifyTexture );
	if ( s_LockedTexture || !pTexture || level < 0 || level > 30 || cubeFaceID != 0 || width <= 0 || height <= 0 ) return false;
	int mipWidth = MAX( 1, pTexture->width >> level ), mipHeight = MAX( 1, pTexture->height >> level );
	if ( xOffset < 0 || yOffset < 0 || width > mipWidth || height > mipHeight || xOffset > mipWidth-width || yOffset > mipHeight-height ) return false;
	s_TextureLockBits.EnsureCapacity( width * height * 4 );
	if ( level != 0 ) return false;
	if ( !pTexture->lockShadow )
	{
		pTexture->lockShadow = new unsigned char[pTexture->width * pTexture->height * 4];
		memset( pTexture->lockShadow, 0, pTexture->width * pTexture->height * 4 );
	}
	for ( int row = 0; row < height; ++row )
		memcpy( s_TextureLockBits.Base() + row * width * 4,
			pTexture->lockShadow + ( ( yOffset + row ) * pTexture->width + xOffset ) * 4, width * 4 );
	s_LockedTexture = s_ModifyTexture;
	s_LockLevel = level; s_LockX = xOffset; s_LockY = yOffset;
	s_LockWidth = width; s_LockHeight = height;
	writer.SetPixelMemory( IMAGE_FORMAT_RGBA8888, s_TextureLockBits.Base(), width * 4 );
	static int s_nLockSamples = 0;
	if ( s_nLockSamples++ < 4 )
		Msg( "[GL] Texture lock ready: texture=%u size=%dx%d\n", s_LockedTexture, width, height );
	return true;
}

void CShaderAPIGL::TexUnlock()
{
	if ( !s_LockedTexture ) return;
	GLTextureRecord_t *pTexture = FindGLTexture( s_LockedTexture );
	if ( pTexture && pTexture->lockShadow )
	{
		for ( int row = 0; row < s_LockHeight; ++row )
			memcpy( pTexture->lockShadow + ( ( s_LockY + row ) * pTexture->width + s_LockX ) * 4,
				s_TextureLockBits.Base() + row * s_LockWidth * 4, s_LockWidth * 4 );
	}
	GLint previous;
	glGetIntegerv( GL_TEXTURE_BINDING_2D, &previous );
	glBindTexture( GL_TEXTURE_2D, s_LockedTexture );
	glTexSubImage2D( GL_TEXTURE_2D, s_LockLevel, s_LockX, s_LockY, s_LockWidth, s_LockHeight, GL_RGBA, GL_UNSIGNED_BYTE, s_TextureLockBits.Base() );
	glBindTexture( GL_TEXTURE_2D, previous );
	s_LockedTexture = 0;
}


// These are bound to the texture, not the texture environment
void CShaderAPIGL::TexMinFilter( ShaderTexFilterMode_t texFilterMode )
{
	GLenum filter = GL_LINEAR;
	switch ( texFilterMode )
	{
	case SHADER_TEXFILTERMODE_NEAREST: filter = GL_NEAREST; break;
	case SHADER_TEXFILTERMODE_LINEAR: filter = GL_LINEAR; break;
	case SHADER_TEXFILTERMODE_NEAREST_MIPMAP_NEAREST: filter = GL_NEAREST_MIPMAP_NEAREST; break;
	case SHADER_TEXFILTERMODE_LINEAR_MIPMAP_NEAREST: filter = GL_LINEAR_MIPMAP_NEAREST; break;
	case SHADER_TEXFILTERMODE_NEAREST_MIPMAP_LINEAR: filter = GL_NEAREST_MIPMAP_LINEAR; break;
	case SHADER_TEXFILTERMODE_LINEAR_MIPMAP_LINEAR: filter = GL_LINEAR_MIPMAP_LINEAR; break;
	default: filter = GL_LINEAR; break;
	}

	GLint maxLevel = 1000;
	glGetTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &maxLevel );
	if ( maxLevel == 0 )
	{
		if ( filter == GL_NEAREST_MIPMAP_NEAREST || filter == GL_NEAREST_MIPMAP_LINEAR )
			filter = GL_NEAREST;
		else if ( filter == GL_LINEAR_MIPMAP_NEAREST || filter == GL_LINEAR_MIPMAP_LINEAR )
			filter = GL_LINEAR;
	}

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter );
}

void CShaderAPIGL::TexMagFilter( ShaderTexFilterMode_t texFilterMode )
{
	GLenum filter = ( texFilterMode == SHADER_TEXFILTERMODE_NEAREST ) ? GL_NEAREST : GL_LINEAR;
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter );
}

void CShaderAPIGL::TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode )
{
	GLenum wrap = GL_REPEAT;
	switch ( wrapMode )
	{
	case SHADER_TEXWRAPMODE_CLAMP: wrap = GL_CLAMP_TO_EDGE; break;
	case SHADER_TEXWRAPMODE_REPEAT: wrap = GL_REPEAT; break;
	case SHADER_TEXWRAPMODE_BORDER: wrap = GL_CLAMP_TO_EDGE; break;
	default: wrap = GL_REPEAT; break;
	}
	GLenum pname = ( coord == SHADER_TEXCOORD_S ) ? GL_TEXTURE_WRAP_S : GL_TEXTURE_WRAP_T;
	glTexParameteri( GL_TEXTURE_2D, pname, wrap );
}

void CShaderAPIGL::TexSetPriority( int priority )
{
}

ShaderAPITextureHandle_t CShaderAPIGL::CreateTexture(
	int width,
	int height,
	int depth,
	ImageFormat dstImageFormat,
	int numMipLevels,
	int numCopies,
	int flags,
	const char *pDebugName,
	const char *pTextureGroupName )
{
	GLuint tex = 0;
	glGenTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, tex );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, ( numMipLevels > 0 ) ? ( numMipLevels - 1 ) : 0 );
	int w = width, h = height;
	for ( int mip = 0; mip < MAX( 1, numMipLevels ); ++mip )
	{
		glTexImage2D( GL_TEXTURE_2D, mip, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
		w = MAX( 1, w / 2 ); h = MAX( 1, h / 2 );
	}
	GLTextureRecord_t rec = { tex, width, height, 0, 0, NULL };
	s_GLTextures.AddToTail( rec );
	return (ShaderAPITextureHandle_t)tex;
}

// Create a multi-frame texture (equivalent to calling "CreateTexture" multiple times, but more efficient)
void CShaderAPIGL::CreateTextures(
							ShaderAPITextureHandle_t *pHandles,
							int count,
							int width,
							int height,
							int depth,
							ImageFormat dstImageFormat,
							int numMipLevels,
							int numCopies,
							int flags,
							const char *pDebugName,
							const char *pTextureGroupName )
{
	for ( int k = 0; k < count; ++ k )
	{
		pHandles[ k ] = CreateTexture( width, height, depth, dstImageFormat, numMipLevels, numCopies, flags, pDebugName, pTextureGroupName );
	}
}


ShaderAPITextureHandle_t CShaderAPIGL::CreateDepthTexture( ImageFormat renderFormat, int width, int height, const char *pDebugName, bool bTexture )
{
	return 0;
}

void CShaderAPIGL::DeleteTexture( ShaderAPITextureHandle_t textureHandle )
{
	for ( int i = 0; i < s_GLTextures.Count(); ++i )
	{
		if ( s_GLTextures[i].texture != (GLuint)textureHandle ) continue;
		if ( s_GLTextures[i].framebuffer ) glDeleteFramebuffers( 1, &s_GLTextures[i].framebuffer );
		if ( s_GLTextures[i].depthbuffer ) glDeleteRenderbuffers( 1, &s_GLTextures[i].depthbuffer );
		delete[] s_GLTextures[i].lockShadow;
		s_GLTextures.FastRemove( i );
		break;
	}
	if ( textureHandle )
	{
		GLuint tex = (GLuint)textureHandle;
		glDeleteTextures( 1, &tex );
	}
}

bool CShaderAPIGL::IsTexture( ShaderAPITextureHandle_t textureHandle )
{
	return true;
}

bool CShaderAPIGL::IsTextureResident( ShaderAPITextureHandle_t textureHandle )
{
	return false;
}

// stuff that isn't to be used from within a shader
void CShaderAPIGL::ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil, int renderTargetWidth, int renderTargetHeight )
{
	GLbitfield mask = 0;
	if ( bClearColor )
	{
		glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		mask |= GL_COLOR_BUFFER_BIT;
	}
	if ( bClearDepth )
	{
		glDepthMask( GL_TRUE );
		glClearDepthf( 1.0f );
		mask |= GL_DEPTH_BUFFER_BIT;
	}
	if ( bClearStencil )
	{
		glStencilMask( 0xFF );
		mask |= GL_STENCIL_BUFFER_BIT;
	}
	if ( mask )
	{
		GLboolean scissorWasEnabled = glIsEnabled( GL_SCISSOR_TEST );
		if ( scissorWasEnabled )
		{
			glDisable( GL_SCISSOR_TEST );
		}
		glClear( mask );
		if ( scissorWasEnabled )
		{
			glEnable( GL_SCISSOR_TEST );
		}
	}
}

void CShaderAPIGL::ClearBuffersObeyStencil( bool bClearColor, bool bClearDepth )
{
	ClearBuffersObeyStencilEx( bClearColor, bClearColor, bClearDepth );
}

void CShaderAPIGL::ClearBuffersObeyStencilEx( bool bClearColor, bool bClearAlpha, bool bClearDepth )
{
	GLbitfield mask = 0;
	if ( bClearColor || bClearAlpha ) mask |= GL_COLOR_BUFFER_BIT;
	if ( bClearDepth )
	{
		mask |= GL_DEPTH_BUFFER_BIT;
		glClearDepthf( 1.0f );
	}
	if ( mask )
	{
		glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		glDepthMask( GL_TRUE );
		GLboolean scissorWasEnabled = glIsEnabled( GL_SCISSOR_TEST );
		if ( scissorWasEnabled )
		{
			glDisable( GL_SCISSOR_TEST );
		}
		glClear( mask );
		if ( scissorWasEnabled )
		{
			glEnable( GL_SCISSOR_TEST );
		}
	}
}

void CShaderAPIGL::PerformFullScreenStencilOperation( void )
{
}

void CShaderAPIGL::SetScissorRect( const int nLeft, const int nTop, const int nRight, const int nBottom, const bool bEnableScissor )
{
	if ( bEnableScissor && nRight > nLeft && nBottom > nTop )
	{
		int screenW = 1280, screenH = 720;
		GetGLTargetSize( screenW, screenH );
		int scissorY = screenH - nBottom;
		if ( scissorY < 0 )
			scissorY = 0;
		int scissorW = nRight - nLeft;
		int scissorH = nBottom - nTop;
		if ( scissorW > screenW ) scissorW = screenW;
		if ( scissorH > screenH ) scissorH = screenH;

		glEnable( GL_SCISSOR_TEST );
		glScissor( nLeft, scissorY, scissorW, scissorH );
	}
	else
	{
		glDisable( GL_SCISSOR_TEST );
	}
}

void CShaderAPIGL::ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat )
{
}

void CShaderAPIGL::ReadPixels( Rect_t *pSrcRect, Rect_t *pDstRect, unsigned char *data, ImageFormat dstFormat, int nDstStride )
{
}

void CShaderAPIGL::FlushHardware()
{
}

void CShaderAPIGL::ResetRenderState( bool bFullReset )
{
}

// Set the number of bone weights
void CShaderAPIGL::SetNumBoneWeights( int numBones )
{
}

void CShaderAPIGL::EnableHWMorphing( bool bEnable )
{
}

// Selection mode methods
int CShaderAPIGL::SelectionMode( bool selectionMode )
{
	return 0;
}

void CShaderAPIGL::SelectionBuffer( unsigned int* pBuffer, int size )
{
}

void CShaderAPIGL::ClearSelectionNames( )
{
}

void CShaderAPIGL::LoadSelectionName( int name )
{
}

void CShaderAPIGL::PushSelectionName( int name )
{
}

void CShaderAPIGL::PopSelectionName()
{
}


// Use this to get the mesh builder that allows us to modify vertex data
CMeshBuilder* CShaderAPIGL::GetVertexModifyBuilder()
{
	return 0;
}

// Board-independent calls, here to unify how shaders set state
// Implementations should chain back to IShaderUtil->BindTexture(), etc.

// Use this to begin and end the frame
void CShaderAPIGL::BeginFrame()
{
}

void CShaderAPIGL::EndFrame()
{
}

// returns the current time in seconds....
double CShaderAPIGL::CurrentTime() const
{
	return Sys_FloatTime();
}

// Get the current camera position in world space.
void CShaderAPIGL::GetWorldSpaceCameraPosition( float * pPos ) const
{
	if ( !pPos )
		return;
	VMatrix camToWorld;
	if ( MatrixInverseGeneral( m_MatrixStack[MATERIAL_VIEW].Top(), camToWorld ) )
	{
		Vector pos = camToWorld.GetTranslation();
		pPos[0] = pos.x;
		pPos[1] = pos.y;
		pPos[2] = pos.z;
	}
	else
	{
		pPos[0] = pPos[1] = pPos[2] = 0.0f;
	}
}

void CShaderAPIGL::ForceHardwareSync( void )
{
}

void CShaderAPIGL::SetClipPlane( int index, const float *pPlane )
{
}

void CShaderAPIGL::EnableClipPlane( int index, bool bEnable )
{
}

void CShaderAPIGL::SetFastClipPlane( const float *pPlane )
{
}

void CShaderAPIGL::EnableFastClip( bool bEnable )
{
}

int CShaderAPIGL::GetCurrentNumBones( void ) const
{
	return 0;
}

bool CShaderAPIGL::IsHWMorphingEnabled( void ) const
{
	return false;
}

int CShaderAPIGL::GetCurrentLightCombo( void ) const
{
	return 0;
}

void CShaderAPIGL::GetDX9LightState( LightState_t *state ) const
{
	state->m_nNumLights = 0;
	state->m_bAmbientLight = false;
	state->m_bStaticLightVertex = false;
	state->m_bStaticLightTexel = false;
}

MaterialFogMode_t CShaderAPIGL::GetCurrentFogType( void ) const
{
	return MATERIAL_FOG_NONE;
}

void CShaderAPIGL::RecordString( const char *pStr )
{
}

bool CShaderAPIGL::ReadPixelsFromFrontBuffer() const
{
	return true;
}

bool CShaderAPIGL::PreferDynamicTextures() const
{
	return false;
}

bool CShaderAPIGL::PreferReducedFillrate() const
{
	return false;
}

bool CShaderAPIGL::HasProjectedBumpEnv() const
{
	return true;
}

int  CShaderAPIGL::GetCurrentDynamicVBSize( void )
{
	return 1024 * 1024;
}

void CShaderAPIGL::DestroyVertexBuffers( bool bExitingLevel )
{
}

void CShaderAPIGL::EvictManagedResources()
{
}

void CShaderAPIGL::SetTextureTransformDimension( TextureStage_t textureStage, int dimension, bool projected )
{
}

void CShaderAPIGL::SetBumpEnvMatrix( TextureStage_t textureStage, float m00, float m01, float m10, float m11 )
{
}

void CShaderAPIGL::SyncToken( const char *pToken )
{
}
