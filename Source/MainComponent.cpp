

#include "MainComponent.h"


using namespace juce;


WaveMarkerComp::WaveMarkerComp (AudioFormatManager& formatManager,
                                      AudioTransportSource& source,
                                      Slider& slider)
: transportSource (source),
zoomSlider (slider),
thumbnail (512, formatManager, thumbnailCache),
currentPositionMarker(source)
{
  thumbnail.addChangeListener (this);
  
  addAndMakeVisible (scrollbar);
  scrollbar.setRangeLimits (visibleRange);
  scrollbar.setAutoHide (false);
  scrollbar.addListener (this);

  addAndMakeVisible (currentPositionMarker);
  
  addAndMakeVisible (addMarker);
  addMarker.addListener (this);
  
  
  
  setOpaque(true);
}

WaveMarkerComp::~WaveMarkerComp()
{
  scrollbar.removeListener (this);
  thumbnail.removeChangeListener (this);
}

void WaveMarkerComp::setURL (const URL& url)
{
  markersLocation = File();
  InputSource* inputSource = nullptr;
  
#if ! JUCE_IOS
  if (url.isLocalFile())
  {
    inputSource = new FileInputSource (url.getLocalFile());
    markersLocation = url.getLocalFile().getFullPathName() + MarkerFilesExt;
  }
  else
#endif
  {
    if (inputSource == nullptr)
      inputSource = new URLInputSource (url);
  }
  
  if (inputSource != nullptr)
  {
    thumbnail.setSource (inputSource);
    
    Range<double> newRange (0.0, thumbnail.getTotalLength());
    scrollbar.setRangeLimits (newRange);
    setRange (newRange);
    
    startTimerHz (40);
    
    loadMarkers();
    
  }
}

URL WaveMarkerComp::getLastDroppedFile() const noexcept { return lastFileDropped; }

void WaveMarkerComp::setZoomFactor (double amount)
{
  if (thumbnail.getTotalLength() > 0)
  {
    auto newScale = jmax (0.001, thumbnail.getTotalLength() * (1.0 - jlimit (0.0, 0.99, amount)));
    auto timeAtCentre = xToTime (getWidth() / 2.0f);
    
    auto timeAtCursor = xToTime (currentPositionMarker.getX());
    
    setRange ({ juce::jmax(0., timeAtCursor - newScale * 0.5), timeAtCursor + newScale * 0.5 });
  }
}

void WaveMarkerComp::setRange (Range<double> newRange)
{
  visibleRange = newRange;
  scrollbar.setCurrentRange (visibleRange);
  updateCursorPosition();
  repaint();
}

void WaveMarkerComp::setFollowsTransport (bool shouldFollow)
{
  isFollowingTransport = shouldFollow;
}

void WaveMarkerComp::paint (Graphics& g)
{
  g.fillAll (ColorWaveThumbnailBkg);
  
  
  g.setColour (ColorText1);
  g.setFont (20.0f);
  juce::Time time(transportSource.getCurrentPosition()*1000);
  juce::String timeStr = juce::String(time.getMinutes()) + juce::String(":") + juce::String(time.getSeconds()) + juce::String(".") + juce::String(time.getMilliseconds());
  g.drawText(timeStr, 0, 0, getWidth(), addMarker.getHeight(), juce::Justification::centred);
  
  if (thumbnail.getTotalLength() > 0.0)
  {
  g.setColour(ColorWaveThumbnailForm);
  //draw thumb
    auto thumbArea = getLocalBounds().removeFromBottom(getHeight()*0.50);
    
    thumbArea.removeFromBottom (scrollbar.getHeight() + 4);
    thumbnail.drawChannels (g, thumbArea.reduced (2),
                            visibleRange.getStart(), visibleRange.getEnd(), 1.0f);
  }
  else
  {
    g.setFont (14.0f);
    g.drawFittedText ("(Drop audio file here)", getLocalBounds(), Justification::centred, 2);
  }
}

void WaveMarkerComp::resized()
{
  addMarker.setBounds(1, 1, 25, 25);
  scrollbar.setBounds (getLocalBounds().removeFromBottom (14).reduced (2));
  repaint();
}

void WaveMarkerComp::changeListenerCallback (ChangeBroadcaster*)
{
  // this method is called by the thumbnail when it has changed, so we should repaint it..
  repaint();
}

bool WaveMarkerComp::isInterestedInFileDrag (const StringArray& /*files*/)
{
  return true;
}

void WaveMarkerComp::filesDropped (const StringArray& files, int /*x*/, int /*y*/)
{
  lastFileDropped = URL (File (files[0]));
  sendChangeMessage();
}

void WaveMarkerComp::buttonClicked (juce::Button *btn)
{
  if (&addMarker == btn)
  {
    addMarkerToList(transportSource.getCurrentPosition(), "NEW MARKER", "*No Comment*", true);
  }
  else
  {
    for (auto it = markers.begin(); it != markers.end(); ++it)
    {
      if (&(*it)->editMarker == btn)
      {
        (*it)->title = (*it)->editTitle.getText();
        saveMarkers();
      }
      if (&(*it)->delMarker == btn)
      {
        markers.erase(it);
        break;
      }
    }
  }
  resized();
}


void WaveMarkerComp::addMarkerToList(double time, const juce::String &title, const juce::String &desc, bool saveXML)
{
  MarkerInfo *newMarker = new MarkerInfo();
  newMarker->pos = time;
  newMarker->title = title;
  newMarker->desc = desc;
  newMarker->delMarker.addListener(this);
  newMarker->editMarker.addListener(this);

  addChildComponent(newMarker);
  markers.push_back(newMarker);
  resized();
  if (saveXML)
    saveMarkers();
}


void WaveMarkerComp::saveMarkers()
{
  juce::XmlElement root("Markers");
  for (auto &marker: markers)
  {
    auto m = root.createNewChildElement("Marker");
    m->setAttribute("Time", marker->pos);
    m->setAttribute("Title", marker->title);
    m->setAttribute("Desc", marker->desc);
  }
  bool res = root.writeToFile(markersLocation, "");
  jassert(res);
}


void WaveMarkerComp::loadMarkers()
{
  XmlElement *root = XmlDocument::parse(markersLocation);
  if (!root)
    return;
 
  markers.clear();
  
  for (int i = 0; i < root->getNumChildElements(); ++i)
  {
    auto m = root->getChildElement(i);
    if (m->getTagName() == "Marker")
    {
      double time = m->getDoubleAttribute("Time");
      juce::String title = m->getStringAttribute("Title");
      juce::String desc = m->getStringAttribute("Desc");
      addMarkerToList(time, title, desc);
    }
  }
  delete root;
  resized();
}



void WaveMarkerComp::mouseDown (const MouseEvent& e)
{
  mouseDrag (e);
}

void WaveMarkerComp::mouseDrag (const MouseEvent& e)
{
  if (canMoveTransport())
    transportSource.setPosition (jmax (0.0, xToTime ((float) e.x)));
}

void WaveMarkerComp::mouseUp (const MouseEvent&)
{
 // transportSource.start();
}

void WaveMarkerComp::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& wheel)
{
  if (thumbnail.getTotalLength() > 0.0)
  {
    auto newStart = visibleRange.getStart() - wheel.deltaX * (visibleRange.getLength()) / 10.0;
    newStart = jlimit (0.0, jmax (0.0, thumbnail.getTotalLength() - (visibleRange.getLength())), newStart);
    
    if (canMoveTransport())
      setRange ({ newStart, newStart + visibleRange.getLength() });
    
    if (wheel.deltaY != 0.0f)
      zoomSlider.setValue (zoomSlider.getValue() - wheel.deltaY);
    
    repaint();
  }
}


float WaveMarkerComp::timeToX (const double time) const
{
  if (visibleRange.getLength() <= 0)
    return 0;
  
  return getWidth() * (float) ((time - visibleRange.getStart()) / visibleRange.getLength());
}

double WaveMarkerComp::xToTime (const float x) const
{
  return (x / getWidth()) * (visibleRange.getLength()) + visibleRange.getStart();
}

bool WaveMarkerComp::canMoveTransport() const noexcept
{
  return ! (isFollowingTransport && transportSource.isPlaying());
}

void WaveMarkerComp::scrollBarMoved (ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
  if (scrollBarThatHasMoved == &scrollbar)
    if (! (isFollowingTransport && transportSource.isPlaying()))
      setRange (visibleRange.movedToStartAt (newRangeStart));
}

void WaveMarkerComp::timerCallback()
{
  
  repaint(0, 0, getWidth(), addMarker.getHeight());
  
  if (canMoveTransport())
    updateCursorPosition();
  else
    setRange (visibleRange.movedToStartAt (transportSource.getCurrentPosition() - (visibleRange.getLength() / 2.0)));
}

void WaveMarkerComp::updateCursorPosition()
{

  currentPositionMarker.setBounds(timeToX (transportSource.getCurrentPosition()) - 0.75f, addMarker.getBottom(),
                                                        50.f, (float) (getHeight() - scrollbar.getHeight() - addMarker.getBottom()));
  
  if (thumbnail.getTotalLength() > 0.0)
  {
    for (auto &marker : markers)
    {
      if (marker->pos >= visibleRange.getStart() && marker->pos < visibleRange.getEnd())
      {
        float curPos = timeToX(marker->pos) - 0.75f;
        marker->setVisible(true);
        marker->setBounds(curPos, addMarker.getBottom(), 200, getHeight() - scrollbar.getHeight() - addMarker.getBottom());
      }
      else
        marker->setVisible(false);
    }
  }

}
















//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************



















PlayerActionsComponent::PlayerActionsComponent()
{
  addAndMakeVisible (zoomLabel);
  zoomLabel.setFont (Font (15.00f, Font::plain));
  zoomLabel.setJustificationType (Justification::centredRight);
  zoomLabel.setEditable (false, false, false);
  
  addAndMakeVisible (followTransportButton);
  followTransportButton.onClick = [this] { updateFollowTransportState(); };
  
  addAndMakeVisible (zoomSlider);
  zoomSlider.setRange (0, 1, 0);
  zoomSlider.onValueChange = [this] { waveMarkerComp->setZoomFactor (zoomSlider.getValue()); };
  zoomSlider.setSkewFactor (2);
  
  waveMarkerComp.reset (new WaveMarkerComp (formatManager, transportSource, zoomSlider));
  addAndMakeVisible (waveMarkerComp.get());
  waveMarkerComp->addChangeListener (this);
  
  addAndMakeVisible (startPauseButton);
  startPauseButton.onClick = [this] { startOrPause(); };
  
  addAndMakeVisible (stopButton);
  stopButton.onClick = [this] { stop(); };
  
  // audio setup
  formatManager.registerBasicFormats();
  
  thread.startThread (5);
  
  audioDeviceManager.initialise (0, 2, nullptr, true, {}, nullptr);
  
  audioDeviceManager.addAudioCallback (&audioSourcePlayer);
  audioSourcePlayer.setSource (&transportSource);

  setSize (500, 500);
}

PlayerActionsComponent::~PlayerActionsComponent()
{
  transportSource  .setSource (nullptr);
  audioSourcePlayer.setSource (nullptr);
  
  audioDeviceManager.removeAudioCallback (&audioSourcePlayer);

  waveMarkerComp->removeChangeListener (this);
}

void PlayerActionsComponent::paint (Graphics& g)
{
}

void PlayerActionsComponent::resized()
{
  auto r = getLocalBounds().reduced (4);
  
  auto zoom = r.removeFromTop (25);
  zoomLabel .setBounds (zoom.removeFromLeft (50));
  zoomSlider.setBounds (zoom);
  
  auto controls = r.removeFromBottom (25);

  

  startPauseButton      .setBounds (controls.removeFromLeft(80));
  stopButton            .setBounds (controls.removeFromLeft(80));
  followTransportButton.setBounds (controls.removeFromLeft (100));
  waveMarkerComp->setBounds (r);
}



void PlayerActionsComponent::showAudioResource (URL resource)
{
  if (loadURLIntoTransport (resource))
    currentAudioFile = static_cast<URL&&> (resource);
  
  zoomSlider.setValue (0, dontSendNotification);
  waveMarkerComp->setURL (currentAudioFile);
}

bool PlayerActionsComponent::loadURLIntoTransport (const URL& audioURL)
{
  // unload the previous file source and delete it..
  transportSource.stop();
  transportSource.setSource (nullptr);
  currentAudioFileSource.reset();
  
  AudioFormatReader* reader = nullptr;
  
  if (audioURL.isLocalFile())
  {
    reader = formatManager.createReaderFor (audioURL.getLocalFile());
  }
  else
  {
    if (reader == nullptr)
      reader = formatManager.createReaderFor (audioURL.createInputStream (false));
  }
  
  if (reader != nullptr)
  {
    currentAudioFileSource.reset (new AudioFormatReaderSource (reader, true));
    
    // ..and plug it into our transport source
    transportSource.setSource (currentAudioFileSource.get(),
                               32768,                   // tells it to buffer this many samples ahead
                               &thread,                 // this is the background thread to use for reading-ahead
                               reader->sampleRate);     // allows for sample rate correction
    
    return true;
  }
  
  return false;
}


void PlayerActionsComponent::startOrPause()
{
  if (transportSource.isPlaying())
    transportSource.stop();
  else
    transportSource.start();
}

void PlayerActionsComponent::stop()
{
  if (transportSource.isPlaying())
    transportSource.stop();
  transportSource.setPosition (0);
}


void PlayerActionsComponent::updateFollowTransportState()
{
  waveMarkerComp->setFollowsTransport (followTransportButton.getToggleState());
}


void PlayerActionsComponent::selectionChanged()
{
  //showAudioResource (URL (fileTreeComp.getSelectedFile()));
}

void PlayerActionsComponent::fileClicked (const File&, const MouseEvent&)            {}
void PlayerActionsComponent::fileDoubleClicked (const File&)                         {}
void PlayerActionsComponent::browserRootChanged (const File&)                        {}

void PlayerActionsComponent::changeListenerCallback (ChangeBroadcaster* source)
{
  if (source == waveMarkerComp.get())
    showAudioResource (URL (waveMarkerComp->getLastDroppedFile()));
}












//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************
//*********************************************************************************















MainComponent::MainComponent()
{
  getLookAndFeel().setColour(TextEditor::textColourId, ColorText1);
  getLookAndFeel().setColour(Label::textColourId, ColorText1);
  
  getLookAndFeel().setColour(ToggleButton::textColourId, ColorText1);
  
  getLookAndFeel().setColour(ScrollBar::thumbColourId, ColorText1.withAlpha(0.5f));
  
  getLookAndFeel().setColour(Slider::thumbColourId, ColorText1);
  
  getLookAndFeel().setColour(TextEditor::backgroundColourId, ColorDefaultBkg);
  getLookAndFeel().setColour(TextEditor::highlightColourId, ColorText1);
  getLookAndFeel().setColour(TextEditor::textColourId, ColorText1);
  getLookAndFeel().setColour(TextEditor::highlightedTextColourId, ColorText1);
  getLookAndFeel().setColour(TextEditor::outlineColourId, juce::Colours::transparentBlack);
  getLookAndFeel().setColour(TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
  getLookAndFeel().setColour(TextEditor::shadowColourId, juce::Colours::transparentBlack);
  
  playerActionsComponent.reset(new PlayerActionsComponent());
  addAndMakeVisible(playerActionsComponent);
  setSize (600, 250);
}

MainComponent::~MainComponent()
{
}


void MainComponent::paintOverChildren(Graphics& g)
{
  //    StringArray commandArg = juce::JUCEApplication::getCommandLineParameterArray();
  //    String openFile;
  //    if (commandArg.size() > 1)
  //        openFile = commandArg[1];
  //
  //    g.setFont (Font (16.0f));
  //    g.setColour (Colours::white);
  //    g.drawText (juce::JUCEApplication::getCommandLineParameters(), getLocalBounds(), Justification::centred, true);
}

void MainComponent::paint (Graphics& g)
{
  g.fillAll (ColorDefaultBkg);
}

void MainComponent::resized()
{
  playerActionsComponent->setSize(getWidth(), getHeight());
}





