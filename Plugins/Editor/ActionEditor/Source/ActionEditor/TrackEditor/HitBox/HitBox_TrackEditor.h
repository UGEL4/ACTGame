// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneTrack.h"
#include "MovieSceneTrackEditor.h"
#include "ISequencerSection.h"
#include "TimeToPixel.h"

/**
 * 
 */
struct FGuid;
class FMenuBuilder;

class ACTIONEDITOR_API FHitBox_TrackEditor : public FMovieSceneTrackEditor
{
public:

    FHitBox_TrackEditor(TSharedRef<ISequencer> InSequencer);
    ~FHitBox_TrackEditor();

	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer);

public:
    // ISequencerTrackEditor interface
    virtual FText GetDisplayName() const override;
    virtual void BuildAddTrackMenu(FMenuBuilder& MenuBuilder) override;
	virtual bool IsResizable(UMovieSceneTrack* InTrack) const override
    {
        return true;
    }

	virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> TrackClass) const override;
    virtual bool SupportsSequence(UMovieSceneSequence* InSequence) const override;
    /** Gets an icon brush for this track editor */
    virtual const FSlateBrush* GetIconBrush() const override;

    void HandleAddTrackMenuEntryExecute();
    //virtual UMovieSceneTrack* AddTrack(UMovieScene* FocusedMovieScene, const FGuid& ObjectHandle, TSubclassOf<UMovieSceneTrack> TrackClass, FName UniqueTypeName) override;
    bool CanAddTrack() const;

    virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	
};

struct FTimeToPixelWarp : UE::Sequencer::FTimeToPixelSpace
{
public:
    FTimeToPixelWarp()
    {
    }

    FTimeToPixelWarp(const FGeometry& AllottedGeometry, const TRange<double>& InLocalViewRange, const FFrameRate& InTickResolution)
        : UE::Sequencer::FTimeToPixelSpace(AllottedGeometry.GetLocalSize().X, InLocalViewRange, InTickResolution)
    {
    }

    FTimeToPixelWarp(float WidthPx, const TRange<double>& InLocalViewRange, const FFrameRate& InTickResolution)
        : UE::Sequencer::FTimeToPixelSpace(WidthPx, InLocalViewRange, InTickResolution)
    {
    }

    FTimeToPixelWarp(const UE::Sequencer::FTimeToPixelSpace& InOther)
        : UE::Sequencer::FTimeToPixelSpace(InOther)
    {
    }


    FTimeToPixelWarp& operator=(const UE::Sequencer::FTimeToPixelSpace& InOther)
    {
        static_cast<UE::Sequencer::FTimeToPixelSpace&>(*this) = InOther;
        return *this;
    }

    void Update(const UE::Sequencer::FTimeToPixelSpace& InOther)
    {
        static_cast<UE::Sequencer::FTimeToPixelSpace&>(*this) = InOther;
    }
    
    bool IsValid() 
    {
        return Valid;
    }

    friend class FHitBox_SectionEditor;

private:
    bool Valid{ false };
};

class FHitBox_SectionEditor : public FSequencerSection , public TSharedFromThis<FHitBox_SectionEditor>
{
public:
    FHitBox_SectionEditor(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer);
    virtual ~FHitBox_SectionEditor();

public:
    virtual int32 OnPaintSection(FSequencerSectionPainter& Painter) const override;

    virtual void BuildSectionContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding) override;
    virtual TSharedRef<SWidget> GenerateSectionWidget() override;

    virtual FText GetSectionTitle() const override;
    virtual FText GetSectionToolTip() const override;
    virtual float GetSectionHeight() const override;

    virtual bool SectionIsResizable() const { return true; }
    virtual void GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder);

    virtual void BeginResizeSection() override;
    virtual void ResizeSection(ESequencerSectionResizeMode ResizeMode, FFrameNumber ResizeTime) override;

    virtual void BeginSlipSection() override;
    virtual void SlipSection(FFrameNumber SlipTime) override;

private:
    void ShowAddKeyframeDialog();
    void OnAddKeyFrame(FFrameNumber InDisplayFram);

public:
    const FTimeToPixelWarp& GetTimeConverter() const { return TimeToPixelConvertor; }

private:
    TWeakPtr<ISequencer> WeakSequencer;
    mutable FTimeToPixelWarp TimeToPixelConvertor;
};
