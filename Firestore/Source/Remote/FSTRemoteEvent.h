/*
 * Copyright 2017 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import <Foundation/Foundation.h>

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "Firestore/core/src/firebase/firestore/model/document_key.h"
#include "Firestore/core/src/firebase/firestore/model/document_key_set.h"
#include "Firestore/core/src/firebase/firestore/model/snapshot_version.h"
#include "Firestore/core/src/firebase/firestore/model/types.h"
#include "Firestore/core/src/firebase/firestore/remote/remote_event.h"
#include "Firestore/core/src/firebase/firestore/remote/watch_change.h"

@class FSTMaybeDocument;
@class FSTQueryData;

NS_ASSUME_NONNULL_BEGIN

#pragma mark - FSTTargetChange

/**
 * An FSTTargetChange specifies the set of changes for a specific target as part of an
 * FSTRemoteEvent. These changes track which documents are added, modified or emoved, as well as the
 * target's resume token and whether the target is marked CURRENT.
 *
 * The actual changes *to* documents are not part of the FSTTargetChange since documents may be part
 * of multiple targets.
 */
@interface FSTTargetChange : NSObject

/**
 * Creates a new target change with the given SnapshotVersion.
 */
- (instancetype)initWithResumeToken:(NSData *)resumeToken
                            current:(BOOL)current
                     addedDocuments:(firebase::firestore::model::DocumentKeySet)addedDocuments
                  modifiedDocuments:(firebase::firestore::model::DocumentKeySet)modifiedDocuments
                   removedDocuments:(firebase::firestore::model::DocumentKeySet)removedDocuments
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

/**
 * An opaque, server-assigned token that allows watching a query to be resumed after
 * disconnecting without retransmitting all the data that matches the query. The resume token
 * essentially identifies a point in time from which the server should resume sending results.
 */
@property(nonatomic, strong, readonly) NSData *resumeToken;

/**
 * The "current" (synced) status of this target. Note that "current" has special meaning in the RPC
 * protocol that implies that a target is both up-to-date and consistent with the rest of the watch
 * stream.
 */
@property(nonatomic, assign, readonly) BOOL current;

/**
 * The set of documents that were newly assigned to this target as part of this remote event.
 */
- (const firebase::firestore::model::DocumentKeySet &)addedDocuments;

/**
 * The set of documents that were already assigned to this target but received an update during this
 * remote event.
 */
- (const firebase::firestore::model::DocumentKeySet &)modifiedDocuments;

/**
 * The set of documents that were removed from this target as part of this remote event.
 */
- (const firebase::firestore::model::DocumentKeySet &)removedDocuments;

@end

#pragma mark - FSTRemoteEvent

/**
 * An event from the RemoteStore. It is split into targetChanges (changes to the state or the set
 * of documents in our watched targets) and documentUpdates (changes to the actual documents).
 */
@interface FSTRemoteEvent : NSObject

- (instancetype)
    initWithSnapshotVersion:(firebase::firestore::model::SnapshotVersion)snapshotVersion
              targetChanges:
                  (std::unordered_map<firebase::firestore::model::TargetId, FSTTargetChange *>)
                      targetChanges
           targetMismatches:
               (std::unordered_set<firebase::firestore::model::TargetId>)targetMismatches
            documentUpdates:
                (std::unordered_map<firebase::firestore::model::DocumentKey,
                                    FSTMaybeDocument *,
                                    firebase::firestore::model::DocumentKeyHash>)documentUpdates
             limboDocuments:(firebase::firestore::model::DocumentKeySet)limboDocuments;

/** The snapshot version this event brings us up to. */
- (const firebase::firestore::model::SnapshotVersion &)snapshotVersion;

/**
 * A set of which document updates are due only to limbo resolution targets.
 */
- (const firebase::firestore::model::DocumentKeySet &)limboDocumentChanges;

/** A map from target to changes to the target. See TargetChange. */
- (const std::unordered_map<firebase::firestore::model::TargetId, FSTTargetChange *> &)
    targetChanges;

/**
 * A set of targets that is known to be inconsistent. Listens for these targets should be
 * re-established without resume tokens.
 */
- (const std::unordered_set<firebase::firestore::model::TargetId> &)targetMismatches;

/**
 * A set of which documents have changed or been deleted, along with the doc's new values (if not
 * deleted).
 */
- (const std::unordered_map<firebase::firestore::model::DocumentKey,
                            FSTMaybeDocument *,
                            firebase::firestore::model::DocumentKeyHash> &)documentUpdates;

@end

NS_ASSUME_NONNULL_END
