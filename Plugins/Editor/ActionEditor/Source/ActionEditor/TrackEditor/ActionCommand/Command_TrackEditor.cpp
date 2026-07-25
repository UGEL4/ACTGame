// Fill out your copyright notice in the Description page of Project Settings.

#include "Command_TrackEditor.h"
#include "ISequencer.h"
#include "../Track/ActionCommand/Track_Command.h"
#include "../Section/ActionCommand/Section_Command.h"
#include "SequencerSectionPainter.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "FCommand_TrackEditor"

FCommand_TrackEditor::FCommand_TrackEditor(TSharedRef<ISequencer> InSequencer)
    : FMovieSceneTrackEditor(InSequencer)
{
}

FCommand_TrackEditor::~FCommand_TrackEditor()
{
}

TSharedRef<ISequencerTrackEditor> FCommand_TrackEditor::CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer)
{
    return MakeShareable(new FCommand_TrackEditor(OwningSequencer));
}


FText FCommand_TrackEditor::GetDisplayName() const
{
    return LOCTEXT("Command_TrackEditor", "Command");
}

bool FCommand_TrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> TrackClass) const
{
    return TrackClass == UTrack_Command::StaticClass();
}


bool FCommand_TrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
    return true;
}


const FSlateBrush* FCommand_TrackEditor::GetIconBrush() const
{
    return FAppStyle::GetBrush("DebugConsole.Icon");
}


void FCommand_TrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
    // 使用 MenuBuilder 添加一个菜单项
    MenuBuilder.AddMenuEntry(
    LOCTEXT("AddCommandTrack", "Command Track"),
    LOCTEXT("AddCommandTrackTooltip", "Adds a new track that defined the action command."),
    FSlateIcon(), // 可以指定一个图标
    FUIAction(
    FExecuteAction::CreateRaw(this, &FCommand_TrackEditor::HandleAddTrackMenuEntryExecute),
    FCanExecuteAction::CreateSP(this, &FCommand_TrackEditor::CanAddTrack)));
}

void FCommand_TrackEditor::HandleAddTrackMenuEntryExecute()
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

    const FScopedTransaction Transaction(LOCTEXT("AddCommandTrack_Transaction", "Add Command Track"));
    FocusedMovieScene->Modify();

    UTrack_Command* NewTrack = FocusedMovieScene->AddTrack<UTrack_Command>();
    check(NewTrack);

    NewTrack->SetDisplayName(LOCTEXT("CommandTrackName", "Command"));

    if (GetSequencer().IsValid())
    {
        GetSequencer()->OnAddTrack(NewTrack, FGuid());
    }
}

bool FCommand_TrackEditor::CanAddTrack() const
{
    TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
    if (!SequencerPtr.IsValid()) return false;

    UMovieScene* MovieScene = SequencerPtr->GetFocusedMovieSceneSequence()->GetMovieScene();
    if (!MovieScene) return false;

    // 如果已存在则返回 false, 按钮变灰
    for (UMovieSceneTrack* Track : MovieScene->GetTracks())
    {
        if (Track && Track->IsA<UTrack_Command>())
        {
            return false;
        }
    }
    return true;
}

TSharedRef<ISequencerSection> FCommand_TrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
    check(SupportsType(SectionObject.GetOuter()->GetClass()));
    return MakeShared<FCommand_SectionEditor>(SectionObject, GetSequencer());
}

//#region  section
FCommand_SectionEditor::FCommand_SectionEditor(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer)
    : Section(&InSection)
    , Sequencer(InSequencer)
{
}

FCommand_SectionEditor::~FCommand_SectionEditor()
{
}

UMovieSceneSection* FCommand_SectionEditor::GetSectionObject()
{
    return Section.Get();
}

FText FCommand_SectionEditor::GetSectionTitle() const
{
    return FText::FromString("Command");
}

FText FCommand_SectionEditor::GetSectionToolTip() const
{
    return FText::FromString("Define action command info");
}

float FCommand_SectionEditor::GetSectionHeight() const
{
    return 40;
}

int32 FCommand_SectionEditor::OnPaintSection(FSequencerSectionPainter& Painter) const
{
    return Painter.PaintSectionBackground();
}

void FCommand_SectionEditor::BeginResizeSection()
{
}

void FCommand_SectionEditor::ResizeSection(ESequencerSectionResizeMode ResizeMode, FFrameNumber ResizeTime)
{
    ISequencerSection::ResizeSection(ResizeMode, ResizeTime);
}

void FCommand_SectionEditor::GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder)
{
}

void FCommand_SectionEditor::BeginSlipSection()
{
}

void FCommand_SectionEditor::SlipSection(FFrameNumber SlipTime)
{
    ISequencerSection::SlipSection(SlipTime);
}
//#endregion

#undef LOCTEXT_NAMESPACE