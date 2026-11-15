// Rated PG for Pig - main menu and in-game pause menu (hand-drawn UI art, built in code like the other DC widgets).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCMenuWidgets.generated.h"

class UButton;
class UImage;
class UTexture2D;
class UCanvasPanel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCOnMenuButton);

/**
 *  Shared helpers: both menus are laid out on a canvas in pixels of a 1920 x 1080 reference screen
 *  (UMG's default DPI scaling keeps those sizes on other resolutions).
 */
UCLASS(Abstract)
class EPICGAMEJAMGAME_API UDCMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Picture centred on the screen centre + Offset, Size in reference pixels */
	UImage* AddPicture(UCanvasPanel* Canvas, UTexture2D* Texture, const FVector2D& Offset, const FVector2D& Size, int32 ZOrder);

	/** Button drawn with one picture (a little brighter when hovered, darker when pressed) */
	UButton* AddPictureButton(UCanvasPanel* Canvas, UTexture2D* Texture, const FVector2D& Offset, const FVector2D& Size, int32 ZOrder, FName Name);

	/** Full-screen flat colour behind everything */
	UImage* AddBackdrop(UCanvasPanel* Canvas, const FLinearColor& Color);
};

/**
 *  Main menu: the game logo in the upper middle (about 1000 px of visible width) and the START button under it.
 *  ADCMainMenuGameMode shows it and opens the gameplay map on OnStart.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCMainMenuWidget : public UDCMenuWidgetBase
{
	GENERATED_BODY()

public:

	UDCMainMenuWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	TObjectPtr<UTexture2D> LogoTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	TObjectPtr<UTexture2D> StartTexture;

	/** Full-screen picture behind the logo and the START button (Art/UI/MainMenu/MainMenu_BG_A_Framed.png).
	 *  Stretched to the whole screen; drawn above the plain backdrop colour, below the logo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	TObjectPtr<UTexture2D> BackgroundTexture;

	/** Logo size and offset from the screen centre, in 1920 x 1080 reference pixels (the art has a small clear margin) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D LogoSize = FVector2D(1026.0f, 385.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D LogoOffset = FVector2D(0.0f, -150.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D StartSize = FVector2D(326.0f, 65.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D StartOffset = FVector2D(0.0f, 170.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FLinearColor BackdropColor = FLinearColor(0.035f, 0.025f, 0.02f, 1.0f);

	/** OPTIONS button under START (Art/UI/RatedPG/Button_Options.png, 1313 x 439). It only opens the joke card below. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	TObjectPtr<UTexture2D> OptionsTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D OptionsSize = FVector2D(246.0f, 82.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D OptionsOffset = FVector2D(0.0f, 262.0f);

	/** QUIT button under OPTIONS, same bar style (Art/UI/RatedPG/Button_Quit_Bar.png). Closes the game. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	TObjectPtr<UTexture2D> QuitTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D QuitSize = FVector2D(246.0f, 94.0f);      // the art is 1374 x 528

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D QuitOffset = FVector2D(0.0f, 361.0f);

	/** "NOT ENOUGH BUDGET FOR THE OPTIONS" card with the crying pig (Art/UI/RatedPG/Options_NoBudget_Panel.png, 1103 x 851).
	 *  Shown over a dimmed menu; any click, Esc, Enter or Space closes it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	TObjectPtr<UTexture2D> NoBudgetTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D NoBudgetSize = FVector2D(700.0f, 540.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu", meta=(ClampMin=0, ClampMax=1))
	float NoBudgetDimAmount = 0.6f;

	UPROPERTY(BlueprintAssignable, Category="Menu")
	FDCOnMenuButton OnStart;

	UWidget* GetInitialFocusWidget() const;

	/** True while the "not enough budget" card is up */
	bool IsNoBudgetOpen() const { return bNoBudgetOpen; }

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	UFUNCTION()
	void HandleNoBudgetClosed();

	UFUNCTION()
	void HandleQuitClicked();

	void SetNoBudgetOpen(bool bOpen);

	UPROPERTY(Transient)
	TObjectPtr<UButton> StartButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> OptionsButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;

	/** Full-screen dim layer under the card; it is a button so a click anywhere closes the card */
	UPROPERTY(Transient)
	TObjectPtr<UButton> NoBudgetDim;

	UPROPERTY(Transient)
	TObjectPtr<UImage> NoBudgetPanel;

	bool bNoBudgetOpen = false;
};

/**
 *  Pause menu: the game view dimmed, the drawn PAUSED panel in the middle (about 480 px wide) with RESUME and QUIT on it.
 *  ADCPlayerController opens it on Esc and listens to OnResume / OnQuit.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCPauseMenuWidget : public UDCMenuWidgetBase
{
	GENERATED_BODY()

public:

	UDCPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	TObjectPtr<UTexture2D> PanelTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	TObjectPtr<UTexture2D> ResumeTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	TObjectPtr<UTexture2D> QuitTexture;

	/** How dark the game view gets behind the panel (0 = not at all, 1 = black) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu", meta=(ClampMin=0, ClampMax=1))
	float DimAmount = 0.6f;

	/** Panel size in reference pixels (the art is 960 x 1000, so the aspect is kept) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D PanelSize = FVector2D(490.0f, 510.0f);

	/** Button size (the art is 805 x 217) and the two buttons' offsets from the screen centre */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D ButtonSize = FVector2D(300.0f, 81.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D ResumeOffset = FVector2D(0.0f, 40.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FVector2D QuitOffset = FVector2D(0.0f, 135.0f);

	UPROPERTY(BlueprintAssignable, Category="Menu")
	FDCOnMenuButton OnResume;

	UPROPERTY(BlueprintAssignable, Category="Menu")
	FDCOnMenuButton OnQuit;

	UWidget* GetInitialFocusWidget() const;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;
};
