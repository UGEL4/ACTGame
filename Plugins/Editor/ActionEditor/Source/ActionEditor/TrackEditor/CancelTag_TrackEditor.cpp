// Fill out your copyright notice in the Description page of Project Settings.

#include "CancelTag_TrackEditor.h"
#include "ISequencer.h"
#include "../Track/Track_CancelTag.h"
#include "../Section/Section_CancelTag.h"
#include "SequencerSectionPainter.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "FCancelTag_TrackEditor"

FCancelTag_TrackEditor::FCancelTag_TrackEditor(TSharedRef<ISequencer> InSequencer)
    : FMovieSceneTrackEditor(InSequencer)
{
}

FCancelTag_TrackEditor::~FCancelTag_TrackEditor()
{
}

TSharedRef<ISequencerTrackEditor> FCancelTag_TrackEditor::CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer)
{
    return MakeShareable(new FCancelTag_TrackEditor(OwningSequencer));
}


FText FCancelTag_TrackEditor::GetDisplayName() const
{
    return LOCTEXT("CancelTag_TrackEditor", "CancelTag");
}

bool FCancelTag_TrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> TrackClass) const
{
    return TrackClass == UTrack_CancelTag::StaticClass();
}


bool FCancelTag_TrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
    return true;
}


const FSlateBrush* FCancelTag_TrackEditor::GetIconBrush() const
{
    return FAppStyle::GetBrush("DebugConsole.Icon");
}


void FCancelTag_TrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
    // 使用 MenuBuilder 添加一个菜单项
    MenuBuilder.AddMenuEntry(
    LOCTEXT("AddCancelTagTrack", "CancelTag Track"),
    LOCTEXT("AddCancelTagTrackTooltip", "Adds a new track that defined the cancel tag."),
    FSlateIcon(), // 可以指定一个图标
    FUIAction(
    FExecuteAction::CreateRaw(this, &FCancelTag_TrackEditor::HandleAddTrackMenuEntryExecute),
    FCanExecuteAction::CreateSP(this, &FCancelTag_TrackEditor::CanAddTrack)));
}

//void FCancelTag_TrackEditor::BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder,
//                                                         const TArray<FGuid>& ObjectBindings,
//                                                         const UClass* ObjectClass)
//{
//    UMovieSceneSequence* MovieSequence = GetSequencer()->GetFocusedMovieSceneSequence();
//    if (!MovieSequence || MovieSequence->GetClass()->GetName() != TEXT("LevelSequence"))
//    {
//        return;
//    }
//    MenuBuilder.AddMenuEntry(
//    LOCTEXT("AddCancelTagTrack", "CancelTag Track"),
//    LOCTEXT("AddCancelTagTrackTooltip", "Adds a new track that defined the cancel tag."),
//    FSlateIcon(),
//    FUIAction(
//    FExecuteAction::CreateRaw(this, &FCancelTag_TrackEditor::HandleAddTrackMenuEntryExecute, ObjectBindings),
//    FCanExecuteAction::CreateSP(this, &FCancelTag_TrackEditor::CanAddOutlineTrack, ObjectBindings[0])));
//}

void FCancelTag_TrackEditor::HandleAddTrackMenuEntryExecute()
{
    UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

    if (FocusedMovieScene == nullptr)
    {
        return;
    }

    if (FocusedMovieScene->IsReadOnly())
    {
        return;
    }

    const FScopedTransaction Transaction(LOCTEXT("AddCancelTagTrack_Transaction", "Add CancelTag Track"));
    FocusedMovieScene->Modify();

    UTrack_CancelTag* NewTrack = FocusedMovieScene->AddTrack<UTrack_CancelTag>();
    check(NewTrack);

    NewTrack->SetDisplayName(LOCTEXT("CancelTagTrackName", "CancelTag"));

    if (GetSequencer().IsValid())
    {
        GetSequencer()->OnAddTrack(NewTrack, FGuid());
    }
}

bool FCancelTag_TrackEditor::CanAddTrack() const
{
    /*if (UMovieSceneSequence* SceneSequence = GetSequencer()->GetFocusedMovieSceneSequence())
    {
        return SceneSequence->GetMovieScene()->FindTrack<UMovieSceneOutlineTrack>(ObjectBinding) == nullptr;
    }*/
    return true;
}

TSharedRef<ISequencerSection> FCancelTag_TrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
    check(SupportsType(SectionObject.GetOuter()->GetClass()));
    return MakeShared<FSection_CancelTag>(SectionObject, GetSequencer());
}

//#region  section
FSection_CancelTag::FSection_CancelTag(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer)
    : Section(&InSection)
    , Sequencer(InSequencer)
{
}

FSection_CancelTag::~FSection_CancelTag()
{
}

UMovieSceneSection* FSection_CancelTag::GetSectionObject()
{
    return Section.Get();
}

FText FSection_CancelTag::GetSectionTitle() const
{
    return FText::FromString("CancelTag");
}

FText FSection_CancelTag::GetSectionToolTip() const
{
    return FText::FromString("Define cancel tag info");
}

float FSection_CancelTag::GetSectionHeight() const
{
    return 40;
}

int32 FSection_CancelTag::OnPaintSection(FSequencerSectionPainter& Painter) const
{
    return Painter.PaintSectionBackground();
}

void FSection_CancelTag::BeginResizeSection()
{
}

void FSection_CancelTag::ResizeSection(ESequencerSectionResizeMode ResizeMode, FFrameNumber ResizeTime)
{
    ISequencerSection::ResizeSection(ResizeMode, ResizeTime);
}

void FSection_CancelTag::GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder)
{
}

void FSection_CancelTag::BeginSlipSection()
{
}

void FSection_CancelTag::SlipSection(FFrameNumber SlipTime)
{
    ISequencerSection::SlipSection(SlipTime);
}
//#endregion

#undef LOCTEXT_NAMESPACE