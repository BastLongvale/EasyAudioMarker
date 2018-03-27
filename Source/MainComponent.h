/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "list"

#define MarkerFilesExt ".easymarkers"

#define ColorDefaultBkg        Colours::darkgrey
#define ColorWaveThumbnailBkg  Colours::black
#define ColorWaveThumbnailForm Colours::darkgrey
#define ColorWavePlayheadPlay  Colours::orange
#define ColorWavePlayheadStop  Colours::orange
#define ColorWaveMarker         Colours::yellow
#define ColorText1              Colours::white


class MarkerInfo : public juce::Component, private juce::Button::Listener
{
public:
   MarkerInfo()
  {
    setInterceptsMouseClicks(false, true);
    editMarker.setClickingTogglesState(true);
    editMarker.addListener(this);

    addAndMakeVisible(editMarker);
    addAndMakeVisible(delMarker);
  }
  
  void buttonClicked (Button* btn) override
  {
    if (&editMarker == btn)
    {
      if (editMarker.getToggleState())
      {
        editTitle.setText(title);
        editTitle.setReadOnly(false);
        addAndMakeVisible(editTitle);
      }
      else
      {
        title = editTitle.getText();
        removeChildComponent(&editTitle);
      }
    }
  }
  
  
  void resized() override
  {
    editMarker.setBounds(1, 15, 15, 15);
    delMarker.setBounds(1, 31, 15, 15);
    editTitle.setBounds(1, 0, getWidth(), 18);
  }
  
  
  void paint(juce::Graphics &g) override
  {
    g.setColour(ColorWaveMarker);
    if (!editMarker.getToggleState())
      g.drawText(juce::String(title), 1, 0, getWidth(), 20, juce::Justification::topLeft);
    g.drawRect(0, 0, 1, getHeight());
  }
  
  juce::String title;
  juce::String desc;
  double       pos;
  
  TextEditor            editTitle;
  TextButton            editMarker { "E" };
  TextButton            delMarker { "-" };
};



struct PlayHead : public juce::Component
{
  PlayHead (AudioTransportSource &t) : transportSource(t)
  {
    setInterceptsMouseClicks(false, false);
  }
  
  void paint(juce::Graphics &g)
  {
    if (transportSource.isPlaying())
      g.setColour(ColorWavePlayheadPlay);
    else
      g.setColour(ColorWavePlayheadStop);
    g.fillRect(0, 0, 1, getHeight());
  }
private:
  AudioTransportSource &transportSource;
};








class WaveMarkerComp  : public Component,
public ChangeListener,
public FileDragAndDropTarget,
public ChangeBroadcaster,
private ScrollBar::Listener,
private Button::Listener,
private Timer
{
public:
    WaveMarkerComp (AudioFormatManager& formatManager,
                       AudioTransportSource& source,
                       Slider& slider);
    ~WaveMarkerComp();
    void setURL (const URL& url);
    URL getLastDroppedFile() const noexcept;
    void setZoomFactor (double amount);
    void setRange (Range<double> newRange);
    void setFollowsTransport (bool shouldFollow);
    void paint (Graphics& g) override;
    void resized() override;
    void changeListenerCallback (ChangeBroadcaster*) override;
    bool isInterestedInFileDrag (const StringArray& /*files*/) override;
    void filesDropped (const StringArray& files, int /*x*/, int /*y*/) override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent&) override;
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& wheel) override;
    void buttonClicked (Button*) override;
  
  void addMarkerToList(double time, const juce::String &title, const juce::String &desc, bool saveXML = false);
  
  void saveMarkers();
  void loadMarkers();
  
private:
    AudioTransportSource& transportSource;
    Slider&               zoomSlider;
    ScrollBar             scrollbar { false };
    TextButton            addMarker { "+" };
    AudioThumbnailCache   thumbnailCache  { 5 };
    AudioThumbnail        thumbnail;
    Range<double>         visibleRange;
    bool                  isFollowingTransport = false;
    URL                   lastFileDropped;
    PlayHead              currentPositionMarker;
    juce::File            markersLocation;
    std::list<juce::ScopedPointer<MarkerInfo> > markers;
  
    float timeToX (const double time) const;
    double xToTime (const float x) const;
    bool canMoveTransport() const noexcept;
    void scrollBarMoved (ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
    void timerCallback() override;
    void updateCursorPosition();
};





//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************








class PlayerActionsComponent  : public juce::Component, private juce::FileBrowserListener, private juce::ChangeListener
{
public:
    PlayerActionsComponent();
    ~PlayerActionsComponent();
    void paint (Graphics& g) override;
    void resized() override;
    
private:
    AudioDeviceManager audioDeviceManager;
    
    AudioFormatManager formatManager;
    TimeSliceThread thread  { "audio file preview" };
  
    URL currentAudioFile;
    AudioSourcePlayer audioSourcePlayer;
    AudioTransportSource transportSource;
    ScopedPointer<AudioFormatReaderSource> currentAudioFileSource;
    
    ScopedPointer<WaveMarkerComp> waveMarkerComp;
    Label zoomLabel   { {}, "zoom:" };
    Slider zoomSlider                   { Slider::LinearHorizontal, Slider::NoTextBox };
    ToggleButton followTransportButton  { "Follow Transport" };
    TextButton startPauseButton          { "Play/Pause" };
    TextButton stopButton                { "Stop" };
    
    void showAudioResource (URL resource);
    
    bool loadURLIntoTransport (const URL& audioURL);
    
    void startOrPause();
    void stop();
    
    void updateFollowTransportState();
    
    void selectionChanged() override;
    
    void fileClicked (const File&, const MouseEvent&) override ;
    void fileDoubleClicked (const File&) override;
    void browserRootChanged (const File&) override;
    
    void changeListenerCallback (ChangeBroadcaster* source) override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerActionsComponent)
};

//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************


class MainComponent   : public Component
{
public:
    MainComponent();
    ~MainComponent();

     void paint (Graphics&) override;
    void paintOverChildren (Graphics&) override;
    void resized() override;

private:
    juce::ScopedPointer<PlayerActionsComponent> playerActionsComponent;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
