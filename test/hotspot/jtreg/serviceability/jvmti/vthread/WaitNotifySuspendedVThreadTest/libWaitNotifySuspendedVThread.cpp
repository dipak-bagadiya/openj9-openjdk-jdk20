/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include <string.h>
#include "jvmti.h"
#include "jvmti_common.h"

jrawMonitorID monitor;
jrawMonitorID monitor_completed;
static jrawMonitorID events_monitor = NULL;
static const int MAX_EVENTS_TO_PROCESS = 20;
static int vthread_unmount_count = 0;
static int vthread_mount_count = 0;
jvmtiEnv *jvmti_env;


static void
set_breakpoint(JNIEnv *jni, jclass klass, const char *mname) {
  jlocation location = (jlocation)0L;
  jmethodID method = find_method(jvmti_env, jni, klass, mname);
  jvmtiError err;

  if (method == NULL) {
    jni->FatalError("Error in set_breakpoint: not found method");
  }
  err = jvmti_env->SetBreakpoint(method, location);
  check_jvmti_status(jni, err, "set_or_clear_breakpoint: error in JVMTI SetBreakpoint");


}

extern "C" {

JNIEXPORT void JNICALL
Java_WaitNotifySuspendedVThreadTask_setBreakpoint(JNIEnv *jni, jclass klass) {
  jvmtiError err;

  LOG("setBreakpoint: started\n");
  set_breakpoint(jni, klass, "methBreakpoint");

  // Enable Breakpoint events globally
  err = jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_BREAKPOINT, NULL);
  check_jvmti_status(jni, err, "enableEvents: error in JVMTI SetEventNotificationMode: enable BREAKPOINT");

  LOG("setBreakpoint: finished\n");

}

JNIEXPORT void JNICALL
Java_WaitNotifySuspendedVThreadTask_notifyRawMonitors(JNIEnv *jni, jclass klass, jthread thread) {
  LOG("Main thread: suspending virtual and carrier threads\n");

  //check_jvmti_status(jni, jvmti_env->SuspendThread(thread), "SuspendThread thread");
  jthread cthread = get_carrier_thread(jvmti_env, jni, thread);
  //check_jvmti_status(jni, jvmti_env->SuspendThread(cthread), "SuspendThread thread");

  jint cstate = 0;
  jint vstate = 0;
  jvmtiError err;

  err = jvmti_env->GetThreadState(cthread, &cstate);
  err = jvmti_env->GetThreadState(thread, &vstate);
  LOG("Before start ----> carrier threads state : %x and virtual threads state : %x\n",cstate,vstate)

  if(1)
  {	  
      LOG("suspend carrier thread: %d\n",jvmti_env->SuspendThread(cthread));
      err = jvmti_env->GetThreadState(cthread, &cstate);
      err = jvmti_env->GetThreadState(thread, &vstate);
      LOG("carrier threads state : %x and virtual threads state : %x\n",cstate,vstate);

      LOG("suspend virtual thread: %d\n", jvmti_env->SuspendThread(thread));
      err = jvmti_env->GetThreadState(cthread, &cstate);
      err = jvmti_env->GetThreadState(thread, &vstate);
      LOG("carrier threads state : %x and virtual threads state : %x\n",cstate,vstate);

      LOG("waiting for 10 seconds before resume\n");
      sleep(5);

      LOG("resume virtual thread: %d\n",jvmti_env->ResumeThread(thread));
      err = jvmti_env->GetThreadState(cthread, &cstate);
      err = jvmti_env->GetThreadState(thread, &vstate);
      LOG("carrier threads state : %x and virtual threads state : %x\n",cstate,vstate);

      LOG("resume carrier thread: %d\n",jvmti_env->ResumeThread(cthread));
      err = jvmti_env->GetThreadState(cthread, &cstate);
      err = jvmti_env->GetThreadState(thread, &vstate);
      LOG("carrier threads state : %x and virtual threads state : %x\n",cstate,vstate);
  }	   

  if(0)
  {
      LOG("suspend virtual thread: %d\n",jvmti_env->SuspendThread(thread));
      err = jvmti_env->GetThreadState(cthread, &cstate);
      err = jvmti_env->GetThreadState(thread, &vstate);
      LOG("carrier threads state : %d and virtual threads state : %d\n",cstate,vstate);


      LOG("suspend carrier thread: %d\n", jvmti_env->SuspendThread(cthread));
      err = jvmti_env->GetThreadState(cthread, &cstate);
      err = jvmti_env->GetThreadState(thread, &vstate);
      LOG("carrier threads state : %d and virtual threads state : %d\n",cstate,vstate);

      LOG("waiting for 5 seconds before resume\n");
      sleep(5);

      LOG("resume carrier thread: %d\n",jvmti_env->ResumeThread(cthread));
      err = jvmti_env->GetThreadState(cthread, &cstate);
      err = jvmti_env->GetThreadState(thread, &vstate);
      LOG("carrier threads state : %d and virtual threads state : %d\n",cstate,vstate);

      LOG("resume virtual thread: %d\n",jvmti_env->ResumeThread(thread));
      err = jvmti_env->GetThreadState(cthread, &cstate);
      err = jvmti_env->GetThreadState(thread, &vstate);
      LOG("carrier threads state : %d and virtual threads state : %d\n",cstate,vstate);
  }

  //exit(0);

  {
    RawMonitorLocker rml(jvmti_env, jni, monitor);

    LOG("Main thread: calling monitor.notifyAll()\n");
    rml.notify_all();
  }

  RawMonitorLocker completed(jvmti_env, jni, monitor_completed);
  
  //LOG("Main thread: resuming virtual thread\n");
  //check_jvmti_status(jni, jvmti_env->ResumeThread(thread), "ResumeThread thread");

  //LOG("Main thread: before monitor_completed.wait()\n");
  //completed.wait();
  //LOG("Main thread: after monitor_completed.wait()\n");

  //  LOG("Main thread: resuming carrier thread\n");
  //  check_jvmti_status(jni, jvmti_env->ResumeThread(cthread), "ResumeThread cthread");

  LOG("***********TEST CASE FINISH************\n");
}

} // extern "C"

static void JNICALL
Breakpoint(jvmtiEnv *jvmti, JNIEnv* jni, jthread thread,
           jmethodID method, jlocation location) {
  char* mname = get_method_name(jvmti, jni, method);

  if (strcmp(mname, "methBreakpoint") != 0) {
    LOG("FAILED: got  unexpected breakpoint in method %s()\n", mname);
    deallocate(jvmti, jni, (void*)mname);
    fatal(jni, "Error in breakpoint");
    return;
  }
  char* tname = get_thread_name(jvmti, jni, thread);
  const char* virt = jni->IsVirtualThread(thread) ? "virtual" : "carrier";

  {
    RawMonitorLocker rml(jvmti, jni, monitor);

    LOG("Breakpoint: before monitor.wait(): %s in %s thread\n", mname, virt);
    rml.wait();
    LOG("Breakpoint: after monitor.wait(): %s in %s thread\n", mname, virt);
  }

  RawMonitorLocker completed(jvmti, jni, monitor_completed);

  LOG("Breakpoint: calling monitor_completed.notifyAll()\n");
  completed.notify_all();

  deallocate(jvmti, jni, (void*)tname);
  deallocate(jvmti, jni, (void*)mname);
}

/* ============================================================================= */

static void
processVThreadEvent(jvmtiEnv *jvmti, JNIEnv *jni, jthread vthread, const char *event_name) {
  static int vthread_events_cnt = 0;
  
  char* tname = get_thread_name(jvmti, jni, vthread);

  printf("Inside the fun processVThreadEvent vthread %p\n",vthread);

  if (strcmp(event_name, "VirtualThreadEnd") != 0 &&
      strcmp(event_name, "VirtualThreadStart")  != 0) {
    if (vthread_events_cnt++ > MAX_EVENTS_TO_PROCESS) {
      return; // No need to test all events
    }
  }
  //LOG("processVThreadEvent: event: %s, thread: %s\n", event_name, tname);

 }

static void JNICALL
VirtualThreadStart(jvmtiEnv *jvmti, JNIEnv *jni, jthread vthread) {
  printf("inside the fun VirtualThreadStart\n");
  RawMonitorLocker rml(jvmti, jni, events_monitor);
  processVThreadEvent(jvmti, jni, vthread, "VirtualThreadStart");
}

static void JNICALL
VirtualThreadEnd(jvmtiEnv *jvmti, JNIEnv *jni, jthread vthread) {
  printf("inside the fun VirtualThreadEnd\n");
  RawMonitorLocker rml(jvmti, jni, events_monitor);
  processVThreadEvent(jvmti, jni, vthread, "VirtualThreadEnd");
}

// Parameters: (jvmtiEnv *jvmti, JNIEnv* jni, jthread thread)
static void JNICALL
VirtualThreadMount(jvmtiEnv *jvmti, ...) {
  va_list ap;
  JNIEnv* jni = NULL;
  jthread thread = NULL;

  printf("Inside the fun VirtualThread-Mount\n");

  va_start(ap, jvmti);
  jni = va_arg(ap, JNIEnv*);
  thread = va_arg(ap, jthread);
  va_end(ap);

  RawMonitorLocker rml(jvmti, jni, events_monitor);
  vthread_mount_count++;
  processVThreadEvent(jvmti, jni, thread, "VirtualThreadMount");
}

// Parameters: (jvmtiEnv *jvmti, JNIEnv* jni, jthread thread)
static void JNICALL
VirtualThreadUnmount(jvmtiEnv *jvmti, ...) {
  va_list ap;
  JNIEnv* jni = NULL;
  jthread thread = NULL;

  printf("Inside the fun VirtualThread-Unmount\n");

  va_start(ap, jvmti);
  jni = va_arg(ap, JNIEnv*);
  thread = va_arg(ap, jthread);
  va_end(ap);
  RawMonitorLocker rml(jvmti, jni, events_monitor);
  vthread_unmount_count++;
  processVThreadEvent(jvmti, jni, thread, "VirtualThreadUnmount");
}

JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {
  jvmtiEnv * jvmti = NULL;

  jvmtiEventCallbacks callbacks;
  jvmtiCapabilities caps;
  jvmtiError err;
  jint res;

  res = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_1);
  if (res != JNI_OK || jvmti == NULL) {
    LOG("Wrong result of a valid call to GetEnv!\n");
    return JNI_ERR;
  }

  jvmti_env = jvmti;
  monitor = create_raw_monitor(jvmti, "Monitor");
  monitor_completed = create_raw_monitor(jvmti, "Monitor Completed");

  /* add capability to generate compiled method events */
  memset(&caps, 0, sizeof(jvmtiCapabilities));
  caps.can_support_virtual_threads = 1;
  caps.can_generate_breakpoint_events = 1;
  caps.can_suspend = 1;
  caps.can_access_local_variables = 1;

  err = jvmti->AddCapabilities(&caps);
  if (err != JVMTI_ERROR_NONE) {
     LOG("(AddCapabilities) unexpected error: %s (%d)\n",
            TranslateError(err), err);
     return JNI_ERR;
  }

  err = jvmti->GetCapabilities(&caps);
  if (err != JVMTI_ERROR_NONE) {
    LOG("(GetCapabilities) unexpected error: %s (%d)\n",
          TranslateError(err), err);
    return JNI_ERR;
  }

  /*set event callback */
  LOG("setting event callbacks ...\n");
  (void) memset(&callbacks, 0, sizeof(jvmtiEventCallbacks));
  callbacks.Breakpoint = &Breakpoint;
  callbacks.VirtualThreadStart = &VirtualThreadStart;
  callbacks.VirtualThreadEnd = &VirtualThreadEnd;

  err = jvmti->SetEventCallbacks(&callbacks, sizeof(jvmtiEventCallbacks));
  if (err != JVMTI_ERROR_NONE) {
    LOG("(SetEventCallbacks) unexpected error: %s (%d)\n",
           TranslateError(err), err);
    return JNI_ERR;
  } 
  
  //memset(&callbacks, 0, sizeof(callbacks));
  //callbacks.VirtualThreadStart = &VirtualThreadStart;
  //callbacks.VirtualThreadEnd = &VirtualThreadEnd;

  err = set_ext_event_callback(jvmti, "VirtualThreadMount", VirtualThreadMount);
  if (err != JVMTI_ERROR_NONE) {
    LOG("Agent_OnLoad: Error in JVMTI SetExtEventCallback for VirtualThreadMount: %s(%d)\n",
           TranslateError(err), err);
    return JNI_ERR;
  }

  err = set_ext_event_callback(jvmti, "VirtualThreadUnmount", VirtualThreadUnmount);
  if (err != JVMTI_ERROR_NONE) {
    LOG("Agent_OnLoad: Error in JVMTI SetExtEventCallback for VirtualThreadUnmount: %s(%d)\n",
           TranslateError(err), err);
    return JNI_ERR;
  }

  //memset(&caps, 0, sizeof(caps));
  //caps.can_support_virtual_threads = 1;
  //caps.can_access_local_variables = 1;
  //caps.can_suspend = 1;

  // err = jvmti->AddCapabilities(&caps);
  // if (err != JVMTI_ERROR_NONE) {
  //   LOG("error in JVMTI AddCapabilities: %d\n", err);
  //   return JNI_ERR;
  // }

  // err = jvmti->SetEventCallbacks(&callbacks, sizeof(jvmtiEventCallbacks));
  // if (err != JVMTI_ERROR_NONE) {
  //   LOG("error in JVMTI SetEventCallbacks: %d\n", err);
  //   return JNI_ERR;
  // }

  err = set_ext_event_notification_mode(jvmti, JVMTI_ENABLE, "VirtualThreadMount", NULL);
  if (err != JVMTI_ERROR_NONE) {
    LOG("error in JVMTI SetEventNotificationMode: %d\n", err);
    return JNI_ERR;
  }

  err = set_ext_event_notification_mode(jvmti, JVMTI_ENABLE, "VirtualThreadUnmount", NULL);
  if (err != JVMTI_ERROR_NONE) {
    LOG("error in JVMTI SetEventNotificationMode: %d\n", err);
    return JNI_ERR;
  }

  events_monitor = create_raw_monitor(jvmti, "Events Monitor");

  return JNI_OK;
}
