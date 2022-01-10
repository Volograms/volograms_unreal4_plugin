/**
 * Vologram-playing Actor Class for Unreal 4.
 *
 * Authors:   Anton Gerdelan <anton@volograms.com> \n
 * Copyright: 2021, Volograms (http://volograms.com/) \n
 * Language:  C++ \n
 * Licence:   The MIT License. See LICENSE.md for details. \n
 */

/* NOTES(Anton)
 * Guides/tutorials on U4's Procedural Mesh:
 * https://www.orfeasel.com/creating-procedural-meshes/
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h" // NOTE(Anton) manually added here _before_ the generated.h, which must go last.
#include "vol_geom.h"                // vologram geometry
#include "vol_av.h"                  // libav wrapper
#include "VologramActor.generated.h" // NOTE(Anton) must be included last

// NOTE(Anton) API macro here has the _module_ name, not the class name.
UCLASS()
class VOLOGRAMS_API AVologramActor : public AActor {
  GENERATED_BODY()

  public:
  // Sets default values for this actor's properties
  AVologramActor();

  // This override is just used to reset the actor.
  // NOTE(Anton) this function gets called:
  // * When a new actor is dragged into the scene.
  // * When any single setting in an actor's panel is changed. e.g. When the VOL header file path is changed, but without waiting for the sequence file path
  // change.
  // * Whenever the actor is moved in the editor scene.
  // TODO(Anton) create a stand-in cube or bounding box here so it's easy to visualise/click in the editor.
  virtual void OnConstruction( const FTransform& Transform ) override;

  protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

  // NOTE(Anton) First components manually added for the proc mesh.
  // UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
  UProceduralMeshComponent* proc_mesh_ptr;
  TArray<FVector> vertices, normals;
  TArray<FVector2D> uvs;
  TArray<FLinearColor> vertex_colours;
  TArray<FProcMeshTangent> tangents;
  TArray<int32> triangles;
  void ClearMeshData();
  void ClearIntermediateMeshData();

  /** Loads metadata about a vologram. Should be called once per vologram before playback.
   * @returns                - False if the vologram couldn't be loaded.
   */
  bool load_vologram_meta();

  /** Play a particular frame number for a vologram.
   * @param frame_idx        - Note frames start at 0.
   * @param only_if_keyframe - Ignore loading if not a keyframe. Allows frameskip except over keyframes.
   * @returns                - False if the vologram couldn't be loaded.
   */
  bool update_mesh_with_frame( int frame_idx, bool only_if_keyframe );

  /** Reads the next video frame as an image and copy into texture_ptr to update the texture. */
  void read_next_av_frame_to_texture();

  /** Meta-data about vologram being played. */
  vol_geom_info_t vol_geom_info;
  /** Meta-data about video being played. */
  vol_av_video_t vol_video_info;

  // TODO(Anton) check if it works if i remove this material - not using it any more as we have dynamic material.
  UPROPERTY( EditAnywhere, Category = "Volograms" )
  UMaterialInterface* Material;

  UPROPERTY( EditAnywhere, Category = "Volograms" )
  /** Pointer to custom vologram texture updated from the video. */
  UTexture2D* texture_ptr = NULL;

  // NOTE(Anton) path helpers in FPaths class: https://docs.unrealengine.com/en-US/API/Runtime/Core/Misc/FPaths/index.html
  // NOTE(Anton) directory path also exists: FDirectoryPath
  UPROPERTY( EditAnywhere, Category = "Volograms", DisplayName = "VOL header file" )
  FFilePath vol_header_path;
  UPROPERTY( EditAnywhere, Category = "Volograms", DisplayName = "VOL sequence file" )
  FFilePath vol_sequence_path;
  UPROPERTY( EditAnywhere, Category = "Volograms", DisplayName = "VOL video file" )
  FFilePath vol_mp4_path;
  // UPROPERTY( EditAnywhere, Category = "Volograms" )
  bool vol_meta_info_loaded = false;
  // UPROPERTY( EditAnywhere, Category = "Volograms" )
  bool loaded_first_frame = false;

  double fps = 30.0;

  // UPROPERTY( EditAnywhere, Category = "Volograms" )
  /** Time elapsed within current frame of playback, in seconds. */
  double frame_timer_s = 0.0;
  // UPROPERTY( EditAnywhere, Category = "Volograms" )
  int current_frame = 0;
  // UPROPERTY( EditAnywhere, Category = "Volograms" )
  int previous_keyframe_loaded = -1;
  // UPROPERTY( EditAnywhere, Category = "Volograms" )
  int previous_frame_loaded = -1;
  UPROPERTY( EditAnywhere, Category = "Volograms", DisplayName = "Playing" )
  bool playing = true;
  UPROPERTY( EditAnywhere, Category = "Volograms", DisplayName = "Loop playback" )
  bool loop_vologram = true;

  public:
  // Called every frame
  virtual void Tick( float DeltaTime ) override;
};
