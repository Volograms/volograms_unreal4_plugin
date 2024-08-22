/**
 * Vologram-playing Actor Class for Unreal 4.
 *
 * Authors:   Anton Gerdelan <anton@volograms.com> \n
 * Copyright: 2022, Volograms (http://volograms.com/) \n
 * Language:  C++ \n
 * Licence:   The MIT License. See LICENSE.md for details. \n
 */

#include "VologramActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <string.h>
#include <stdio.h>
#include <string.h>

// Sets default values
AVologramActor::AVologramActor() {
  // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

  // NOTE(Anton) manually added to set up proc mesh
  proc_mesh_ptr = CreateDefaultSubobject<UProceduralMeshComponent>( "CustomMesh" );
  SetRootComponent( proc_mesh_ptr );
  proc_mesh_ptr->bUseAsyncCooking = true;
}

void AVologramActor::ClearMeshData() {
  ClearIntermediateMeshData();

  triangles.Empty();
  uvs.Empty();
}

void AVologramActor::ClearIntermediateMeshData() {
  vertices.Empty();
  normals.Empty();
  vertex_colours.Empty();
  tangents.Empty();
}

bool AVologramActor::load_vologram_meta() {
  // NOTE(Anton) be careful with these Unreal strings - if you dereference the wrong type of string in a UE_LOG it _will_ crash.
  FString hdr_fstr = this->vol_header_path.FilePath;
  FString seq_fstr = this->vol_sequence_path.FilePath;
  FString mp4_fstr = this->vol_mp4_path.FilePath;

  // doing a weird round-about string copy because of temporary memory problems with pointers into FStrings.
  char mp4_char_array[2048], hdr_char_array[2048], seq_char_array[2048];
  mp4_char_array[0] = hdr_char_array[0] = seq_char_array[0] = '\0';
  strncat( hdr_char_array, TCHAR_TO_ANSI( *hdr_fstr ), 2047 );
  strncat( seq_char_array, TCHAR_TO_ANSI( *seq_fstr ), 2047 );
  strncat( mp4_char_array, TCHAR_TO_ANSI( *mp4_fstr ), 2047 );

  if ( loaded_first_frame || this->current_frame != 0 ) { vol_av_close( &this->vol_video_info ); }
  this->loaded_first_frame   = false;
  this->vol_meta_info_loaded = false;
  this->current_frame        = 0;
  this->frame_timer_s        = 0.0;

  { // VIDEO
    bool res = vol_av_open( mp4_char_array, &this->vol_video_info );
    if ( !res ) {
      this->vol_meta_info_loaded = false;
      UE_LOG( LogClass, Warning, TEXT( "[VOL] ERROR: loading VOL MP4 file: `%s`." ), *mp4_fstr );
      return false;
    } else {
      this->vol_meta_info_loaded = true;
      UE_LOG( LogTemp, Log, TEXT( "[VOL] Loaded VOL MP4 file: `%s`" ), *mp4_fstr );
      // vol_av_seek_frame(&this->vol_video_info, -1, 0);
    }
    read_next_av_frame_to_texture();

    this->fps = vol_av_frame_rate( &this->vol_video_info ); // fetch in case it's not 30 or has changed (sometimes 29.97 or so)
    if ( fps <= 0.0 ) { fps = 30.0; }                       // if video reports invalid FPS then guess that it should be 30.
  }
  { // GEOMETRY
		bool streaming_mode = true;
    bool res = vol_geom_create_file_info( hdr_char_array, seq_char_array, &this->vol_geom_info, streaming_mode );
    if ( !res ) {
      this->vol_meta_info_loaded = false;
      // Note that using ASCII string here (despite it using printf formatting) produces gibberish so using original strings
      // if we need to check the ascii contents we could do a windows-style debug message and capture in debugview or write to a file
      UE_LOG( LogClass, Warning, TEXT( "[VOL] ERROR: loading VOL from files. %s %s" ), *hdr_fstr, *seq_fstr );
      return false;
    } else {
      this->vol_meta_info_loaded = true;
      UE_LOG( LogTemp, Log, TEXT( "[VOL] Vologram info loaded from header:`%s` sequence:`%s`\n" ), *hdr_fstr, *seq_fstr );
    }
  }

  return true;
}

bool AVologramActor::update_mesh_with_frame( int frame_idx, bool only_if_keyframe ) {
  // I guess this is a bit of a hack here.
  if ( !this->vol_meta_info_loaded ) {
    if ( !load_vologram_meta() ) {
      UE_LOG( LogTemp, Log, TEXT( "[VOL] ERROR: Failed to load Vologram metadata from files.\n" ) );
      return false;
    }

    // Only set this once to avoid a loop of insanity
    { // ROTATE AND SCALE VOL

      float tx = vol_geom_info.hdr.translation[0];
      float ty = vol_geom_info.hdr.translation[1];
      float tz = vol_geom_info.hdr.translation[2];

      float rx = vol_geom_info.hdr.rotation[1];
      float ry = vol_geom_info.hdr.rotation[2];
      float rz = vol_geom_info.hdr.rotation[3];
      float rw = vol_geom_info.hdr.rotation[0];

      const float u4_scale = 100.0f; // m in .vols to cm in U4

      FTransform T = FTransform( FVector( tz, -tx, ty ) );
      FTransform R;
      R.SetRotation( FQuat( rz, -rx, -rw, ry ) ); // NB(Anton) note that z and w are flipped here...
      FTransform S;
      S.SetScale3D( FVector( vol_geom_info.hdr.scale, vol_geom_info.hdr.scale, vol_geom_info.hdr.scale ) );

      FTransform RST = S * R * T; // left first then right
      FTransform Su;
      Su.SetScale3D( FVector( u4_scale, u4_scale, u4_scale ) );

      FTransform pM = GetActorTransform();
      SetActorTransform( RST * Su * pM );
    }
  }

  if ( frame_idx < 0 || frame_idx >= this->vol_geom_info.hdr.frame_count ) {
    FString TestHUDString = FString( TEXT( "[VOL] ERROR vol_geom_info.hdr.frame_count" ) );
    // GEngine->AddOnScreenDebugMessage( -1, 5.f, FColor::Red, TestHUDString );
    return false;
  } // endif out of range

  bool is_keyframe = ( vol_geom_info.frame_headers_ptr[frame_idx].keyframe != 0 );
  if ( only_if_keyframe && !is_keyframe ) { return true; } // frameskip/drop (can't skip keyframes)
  if ( is_keyframe ) {
    ClearMeshData();
  } else {
    ClearIntermediateMeshData();
  }

  // NOTE(Anton) be careful with these Unreal strings - if you dereference the wrong type of string in a UE_LOG it _will_ crash.
  FString hdr_fstr = this->vol_header_path.FilePath;
  FString seq_fstr = this->vol_sequence_path.FilePath;

  // Doing a weird round-about string copy because of temporary memory problems with pointers into FStrings.
  char hdr_char_array[2048], seq_char_array[2048];
  hdr_char_array[0] = seq_char_array[0] = '\0';
  strncat( hdr_char_array, TCHAR_TO_ANSI( *hdr_fstr ), 2047 );
  strncat( seq_char_array, TCHAR_TO_ANSI( *seq_fstr ), 2047 );

  vol_geom_frame_data_t frame_data = { 0 };
  bool res                         = vol_geom_read_frame( seq_char_array, &this->vol_geom_info, frame_idx, &frame_data );
  if ( !res ) {
    UE_LOG( LogClass, Log, TEXT( "[VOL] ERROR: loading VOL from files. %s %s" ), *hdr_fstr, *seq_fstr );
    return false;
  }
  int n_vertices = frame_data.vertices_sz / ( sizeof( float ) * 3 );

  float* points_ptr = (float*)&frame_data.block_data_ptr[frame_data.vertices_offset];
  vertices.Reserve( n_vertices );
  for ( int i = 0; i < n_vertices; i++ ) {
    float x = points_ptr[i * 3 + 0];
    float y = points_ptr[i * 3 + 1];
    float z = points_ptr[i * 3 + 2];
    // Note ordering change for U4. .vols uses Unity  {+x right,       +y up,    +z into screen} axes.
    //                                         Unreal {+x into screen, +y right, +z up}.
    //                                 Typical OpenGL {+x right,       +y up,    +z out of screen}.
    vertices.Add( FVector( z, x, y ) );
  }
  normals.Reserve( n_vertices );
  vertex_colours.Reserve( n_vertices );
  if ( this->vol_geom_info.hdr.normals && this->vol_geom_info.hdr.version >= 11 ) {
    float* normals_ptr = (float*)&frame_data.block_data_ptr[frame_data.normals_offset];
    for ( int i = 0; i < n_vertices; i++ ) {
      float x = normals_ptr[i * 3 + 0];
      float y = normals_ptr[i * 3 + 1];
      float z = normals_ptr[i * 3 + 2];
      // NOTE(Anton) 14 Jan 2022 updated component order here to match vertex order as per latest vologram reconstructions.
      normals.Add( FVector( z, x, y ) );
      vertex_colours.Add( FLinearColor( z, x, y ) );
    }
  }

  uint8_t indices_type = 0;
  if ( is_keyframe ) {
    uint8_t* uv_byte_ptr = &frame_data.block_data_ptr[frame_data.uvs_offset];
    float* texcoords_ptr = (float*)uv_byte_ptr; // NOTE(Anton) potential alignment issue here with 4-byte floats

    uvs.Reserve( n_vertices );
    for ( int i = 0; i < n_vertices; i++ ) {
      uvs.Add( FVector2D( texcoords_ptr[i * 2 + 0], 1.0f - texcoords_ptr[i * 2 + 1] ) ); // NOTE(Anton) 1.0 - here to flip UV convention for U4.
    }

    indices_type  = n_vertices >= 65535 ? 2 : 1; // uint (2), or ushort (1) is used for small meshes (or old versions of Unity).
    int n_indices = 0;
    if ( indices_type == 1 ) {
      n_indices = frame_data.indices_sz / sizeof( uint16_t );
    } else {
      n_indices = frame_data.indices_sz;
    }

    if ( indices_type == 1 ) {
      uint16_t* indices_short_ptr = (uint16_t*)&frame_data.block_data_ptr[frame_data.indices_offset];
      for ( int i = 0; i < n_indices / 3; i++ ) {
        // NOTE(Anton) reordering from 0,1,2 to 0,2,1 to avoid needing CW winding order (x is mirrored)
        triangles.Add( indices_short_ptr[i * 3 + 0] );
        triangles.Add( indices_short_ptr[i * 3 + 2] );
        triangles.Add( indices_short_ptr[i * 3 + 1] );
      }
    } else {
      uint8_t* indices_byte_ptr = (uint8_t*)&frame_data.block_data_ptr[frame_data.indices_offset];
      for ( int i = 0; i < n_indices / 3; i++ ) {
        // NOTE(Anton) reordering from 0,1,2 to 0,2,1 to avoid needing CW winding order (x is mirrored)
        triangles.Add( indices_byte_ptr[i * 3 + 0] );
        triangles.Add( indices_byte_ptr[i * 3 + 2] );
        triangles.Add( indices_byte_ptr[i * 3 + 1] );
      }
    }
  }

  // Function that creates mesh section
  tangents.Init( FProcMeshTangent( 1.0f, 0.0f, 0.0f ), 3 );
  bool create_collision = false; // probably don't need collisions
  if ( is_keyframe ) {
    proc_mesh_ptr->CreateMeshSection_LinearColor( 0, vertices, triangles, normals, uvs, vertex_colours, tangents, create_collision );
  } else {
    proc_mesh_ptr->UpdateMeshSection_LinearColor( 0, vertices, normals, uvs, vertex_colours, tangents );
  }

  loaded_first_frame = true;

  return true;
}

void AVologramActor::read_next_av_frame_to_texture() {
  if ( !vol_av_read_next_frame( &this->vol_video_info ) ) {
    UE_LOG( LogClass, Warning, TEXT( "[VOL] ERROR loading VOL texture from Mp4" ) );
    return;
  }

  // TODO(Anton) if possible pre-allocate this when loading meta-data and just realloc here if required.
  // HACK TEMP(anton)
  int w            = this->vol_video_info.w;
  int h            = this->vol_video_info.h;
  size_t sz        = (size_t)( this->vol_video_info.w * this->vol_video_info.h * 4 );
  uint8_t* img_ptr = (uint8_t*)malloc( sz );

  // TODO(Anton) have vol_av give you a 4-channel image on demand.
  for ( int y = 0; y < h; y++ ) {
    for ( int x = 0; x < w; x++ ) {
      int idx              = ( y * w + x );
      img_ptr[idx * 4 + 0] = this->vol_video_info.pixels_ptr[idx * 3 + 0];
      img_ptr[idx * 4 + 1] = this->vol_video_info.pixels_ptr[idx * 3 + 1];
      img_ptr[idx * 4 + 2] = this->vol_video_info.pixels_ptr[idx * 3 + 2];
      img_ptr[idx * 4 + 3] = 0xFF;
    }
  }

  if ( !texture_ptr ) {
    texture_ptr = UTexture2D::CreateTransient( w, h, PF_R8G8B8A8 );
    if ( !texture_ptr ) {
      UE_LOG( LogClass, Warning, TEXT( "[VOL] ERROR texture_ptr" ) );
      return;
    }
  }
#if ENGINE_MAJOR_VERSION == 4
  void* textures_data_ptr = texture_ptr->PlatformData->Mips[0].BulkData.Lock( LOCK_READ_WRITE );
  // NOTE(Anton) 4 here, not n!! or U4 will get a corrupted image (that you'll see in the preview)
  FMemory::Memcpy( textures_data_ptr, img_ptr, sz );
  texture_ptr->PlatformData->Mips[0].BulkData.Unlock();
#else 
    void* textures_data_ptr = texture_ptr->GetPlatformData()->Mips[0].BulkData.Lock( LOCK_READ_WRITE );
  // NOTE(Anton) 4 here, not n!! or U4 will get a corrupted image (that you'll see in the preview)
  FMemory::Memcpy( textures_data_ptr, img_ptr, sz );
  texture_ptr->GetPlatformData()->Mips[0].BulkData.Unlock();
#endif

  texture_ptr->UpdateResource();

  free( img_ptr );

  UMaterialInstanceDynamic* MyDMI = proc_mesh_ptr->CreateAndSetMaterialInstanceDynamic( 0 );
  if ( MyDMI ) {
    // NOTE(Anton) to link to texture - right click on texture object in material blueprint and 'convert to parameter' and add this name
    MyDMI->SetTextureParameterValue( FName( "colour" ), texture_ptr );
    // UE_LOG(LogClass, Log, TEXT("[VOL] updated material from texture frame"));
  } else {
    // UE_LOG(LogClass, Warning, TEXT("[VOL] ERROR - updating material from texture frame"));
  }
}

// TODO(ANTON) WARNING -- this will crash if changing path strings one at a time as it gets called on every change to the UI
void AVologramActor::OnConstruction( const FTransform& Transform ) {
  ClearMeshData();
  texture_ptr = NULL; // should be garbage collected
  // unload any previously loaded metadata
  vol_av_close( &this->vol_video_info );
  vol_geom_free_file_info( &this->vol_geom_info );
  current_frame = 0;
}

// Called when the game starts or when spawned
void AVologramActor::BeginPlay() {
  Super::BeginPlay();

  // Can create dynamic material anywhere we like.
  UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create( Material, this );
  // set paramater with Set***ParamaterValue
  // DynMaterial->SetScalarParameterValue("MyParameter", myFloatValue);
  proc_mesh_ptr->SetMaterial( 0, DynMaterial );

  ClearMeshData();
  update_mesh_with_frame( 0, false );
}

// Called every frame
void AVologramActor::Tick( float DeltaTime ) {
  Super::Tick( DeltaTime );

  // INVALID  ( but don't want to print an error every tick )
  if ( this->vol_geom_info.hdr.frame_count < 1 ) { return; }

  if ( !this->playing ) { return; }

  // update timers to see if we should move to the next frame yet
  bool advance_frame = false;
  const double spf   = 1.0 / fps;
  this->frame_timer_s += DeltaTime;
  if ( this->frame_timer_s >= spf ) {
    this->frame_timer_s -= spf;
    advance_frame = true;
  }
  if ( !advance_frame ) { return; }

  // TODO(Anton) add frameskip for really slow playback

  if ( current_frame < this->vol_geom_info.hdr.frame_count - 1 ) {
    current_frame++;
    update_mesh_with_frame( current_frame, false );
    if ( vol_geom_is_keyframe( &this->vol_geom_info, current_frame ) ) { this->previous_keyframe_loaded = current_frame; }
    this->previous_frame_loaded = current_frame;
    read_next_av_frame_to_texture();
  } else if ( this->loop_vologram ) {
    current_frame = 0;

    // have to close and re-open the whole video because it doesn't seek back to 0 properly.
    FString mp4_fstr = this->vol_mp4_path.FilePath;
    char mp4_char_array[2048];
    mp4_char_array[0] = '\0';
    strncat( mp4_char_array, TCHAR_TO_ANSI( *mp4_fstr ), 2047 );
    if ( !vol_av_close( &this->vol_video_info ) ) {
      UE_LOG( LogClass, Warning, TEXT( "[VOL] ERROR: closing VOL MP4 file: `%s`." ), *mp4_fstr );
      return;
    }
    if ( !vol_av_open( mp4_char_array, &this->vol_video_info ) ) {
      UE_LOG( LogClass, Warning, TEXT( "[VOL] ERROR: loading VOL MP4 file: `%s`." ), *mp4_fstr );
      return;
    }
    read_next_av_frame_to_texture();
    // just in case the file changed since last loop!
    this->fps = vol_av_frame_rate( &this->vol_video_info );
    if ( fps <= 0.0 ) { fps = 30.0; }

    update_mesh_with_frame( current_frame, false );
    if ( vol_geom_is_keyframe( &this->vol_geom_info, current_frame ) ) { this->previous_keyframe_loaded = current_frame; }
    this->previous_frame_loaded = current_frame;
  }
}
