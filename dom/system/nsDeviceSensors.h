/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Doug Turner <dougt@dougt.org>
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsDeviceSensors_h
#define nsDeviceSensors_h

#include "nsIDeviceSensors.h"
#include "nsIDOMDeviceMotionEvent.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsIDOMDeviceLightEvent.h"
#include "nsIDOMDeviceOrientationEvent.h"
#include "nsIDOMDeviceProximityEvent.h"
#include "nsIDOMDeviceMotionEvent.h"
#include "nsDOMDeviceMotionEvent.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/HalSensor.h"
#include "nsDataHashtable.h"

#define NS_DEVICE_SENSORS_CID \
{ 0xecba5203, 0x77da, 0x465a, \
{ 0x86, 0x5e, 0x78, 0xb7, 0xaf, 0x10, 0xd8, 0xf7 } }

#define NS_DEVICE_SENSORS_CONTRACTID "@mozilla.org/devicesensors;1"

class nsIDOMWindow;

class nsDeviceSensors : public nsIDeviceSensors, public mozilla::hal::ISensorObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDEVICESENSORS

  nsDeviceSensors();

  virtual ~nsDeviceSensors();

  void Notify(const mozilla::hal::SensorData& aSensorData);

private:
  // sensor -> window listener
  nsTArray<nsTArray<nsIDOMWindow*>* > mWindowListeners;

  void FireDOMLightEvent(nsIDOMEventTarget *target,
                         double value);

  void FireDOMProximityEvent(nsIDOMEventTarget *aTarget,
                             double aValue,
                             double aMin,
                             double aMax);

  void FireDOMOrientationEvent(class nsIDOMDocument *domDoc, 
                               class nsIDOMEventTarget *target,
                               double alpha,
                               double beta,
                               double gamma);

  void FireDOMMotionEvent(class nsIDOMDocument *domDoc, 
                          class nsIDOMEventTarget *target,
                          PRUint32 type,
                          double x,
                          double y,
                          double z);

  bool mEnabled;

  inline bool IsSensorEnabled(PRUint32 aType) {
    return mWindowListeners[aType]->Length() > 0;
  }

  mozilla::TimeStamp mLastDOMMotionEventTime;
  nsRefPtr<nsDOMDeviceAcceleration> mLastAcceleration;
  nsRefPtr<nsDOMDeviceAcceleration> mLastAccelerationIncluduingGravity;
  nsRefPtr<nsDOMDeviceRotationRate> mLastRotationRate;
};

#endif