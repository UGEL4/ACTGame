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

class ACTIONEDITOR_API FActionInfo_TrackEditor : public FMovieSceneTrackEditor
{
public:

	FActionInfo_TrackEditor(TSharedRef<ISequencer> InSequencer);
    ~FActionInfo_TrackEditor();

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
    bool CanAddTrack() const;

    virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	
};

class FActionInfo_SectionEditor : public ISequencerSection, public TSharedFromThis<FActionInfo_SectionEditor>
{
public:
    FActionInfo_SectionEditor(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer);
    virtual ~FActionInfo_SectionEditor();

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
