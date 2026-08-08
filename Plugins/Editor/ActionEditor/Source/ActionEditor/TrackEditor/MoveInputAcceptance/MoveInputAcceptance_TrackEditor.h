// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneTrack.h"
#include "MovieSceneTrackEditor.h"

/**
 * 
 */
struct FGuid;
class FMenuBuilder;

class ACTIONEDITOR_API FMoveInputAcceptance_TrackEditor : public FMovieSceneTrackEditor
{
public:

    FMoveInputAcceptance_TrackEditor(TSharedRef<ISequencer> InSequencer);
    ~FMoveInputAcceptance_TrackEditor();

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

class FMoveInputAcceptance_SectionEditor : public ISequencerSection, public TSharedFromThis<FMoveInputAcceptance_SectionEditor>
{
public:
    FMoveInputAcceptance_SectionEditor(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer);
    ~FMoveInputAcceptance_SectionEditor();

public:
    virtual UMovieSceneSection* GetSectionObject() override;
    virtual int32 OnPaintSection(FSequencerSectionPainter& Painter) const override;

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
    TWeakObjectPtr<UMovieSceneSection> Section;
    TWeakPtr<ISequencer> Sequencer;
};
