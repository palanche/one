/* -------------------------------------------------------------------------- */
/* Copyright 2002-2017, OpenNebula Project, OpenNebula Systems                */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may    */
/* not use this file except in compliance with the License. You may obtain    */
/* a copy of the License at                                                   */
/*                                                                            */
/* http://www.apache.org/licenses/LICENSE-2.0                                 */
/*                                                                            */
/* Unless required by applicable law or agreed to in writing, software        */
/* distributed under the License is distributed on an "AS IS" BASIS,          */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/* See the License for the specific language governing permissions and        */
/* limitations under the License.                                             */
/* -------------------------------------------------------------------------- */

#ifndef VIRTUALNETWORKSTORE_POOL_H
#define VIRTUALNETWORKSTORE_POOL_H

#include "VirtualNetworkStore.h"
#include "SqlDB.h"

using namespace std;


class VirtualNetworkStorePool : public PoolSQL
{
public:
    VirtualNetworkStorePool(SqlDB * db, const vector<const SingleAttribute *>& _inherit_attrs);

    ~VirtualNetworkStorePool(){};

    /* ---------------------------------------------------------------------- */
    /* Constants for DB management                                            */
    /* ---------------------------------------------------------------------- */

    /**
     *  Name for the default VirtualNetworkStore
     */
    static const string DEFAULT_VNS_NAME;

    /**
     *  Identifier for the default VirtualNetworkStore
     */
    static const int DEFAULT_VNS_ID;

    /* ---------------------------------------------------------------------- */
    /* Methods for DB management                                              */
    /* ---------------------------------------------------------------------- */

    /**
     *  Allocates a new VirtualNetworkStore, writing it in the pool database.
     *  No memory is allocated for the object.
     *    @param uid the user id of the Datastore owner
     *    @param gid the id of the group this object is assigned to
     *    @param uname name of the user
     *    @param gname name of the group
     *    @param umask permissions umask
     *    @param vns_template Datastore definition template
     *    @param oid the id assigned to the Datastore
     *    @param cluster_ids the ids of the clusters this Datastore will belong to
     *    @param error_str Returns the error reason, if any
     *
     *    @return the oid assigned to the object, -1 in case of failure
     */
    int allocate(
            int                           uid,
            int                           gid,
            const string&                 uname,
            const string&                 gname,
            int                           umask,
            VirtualNetworkStoreTemplate * vns_template,
            int *                         oid,
            const set<int>                &cluster_ids,
            string&                       error_str);

    /**
     *  Function to get a VirtualNetworkStore from the pool, if the object is
     *  not in memory it is loaded from the DB
     *    @param oid Datastore unique id
     *    @param lock locks the Datastore mutex
     *    @return a pointer to the Datastore, 0 if the Datastore could not be loaded
     */
    VirtualNetworkStore * get(int oid, bool lock)
    {
        return static_cast<VirtualNetworkStore *>(PoolSQL::get(oid,lock));
    };

    /**
     *  Gets an object from the pool (if needed the object is loaded from the
     *  database).
     *   @param name of the object
     *   @param lock locks the object if true
     *
     *   @return a pointer to the object, 0 in case of failure
     */
    VirtualNetworkStore * get(const string& name, bool lock)
    {
        // The owner is set to -1, because it is not used in the key() method
        return static_cast<VirtualNetworkStore *>(PoolSQL::get(name,-1,lock));
    };

    /**
     *  Generate an index key for the object
     *    @param name of the object
     *    @param uid owner of the object, only used if needed
     *
     *    @return the key, a string
     */
    string key(const string& name, int uid)
    {
        // Name is enough key because VirtualNetworkStores can't repeat names.
        return name;
    };

    /**
     *  Drops the VirtualNetworkStore data in the data base. The object mutex
     *  SHOULD be locked.
     *    @param objsql a pointer to the VirtualNetworkStore object
     *    @param error_msg Error reason, if any
     *    @return 0 on success,
     *            -1 DB error,
     *            -3 VirtualNetworkStore's VirtualNetwork IDs set is not empty
     */
    int drop(PoolObjectSQL * objsql, string& error_msg);

    /**
     *  Bootstraps the database table(s) associated to the VirtualNetworkStore
     *  pool
     *    @return 0 on success
     */
    static int bootstrap(SqlDB * _db)
    {
        return VirtualNetworkStore::bootstrap(_db);
    };

    /**
     *  Dumps the VirtualNetworkStore pool in XML format. A filter can be also
     *  added to the query
     *  @param oss the output stream to dump the pool contents
     *  @param where filter for the objects, defaults to all
     *  @param limit parameters used for pagination
     *
     *  @return 0 on success
     */
    int dump(ostringstream& oss, const string& where, const string& limit)
    {
        return PoolSQL::dump(oss, "VIRTUALNETWORKSTORE_POOL",
        VirtualNetworkStore::table, where, limit);
    };

    /**
     *  Lists the VirtualNetworkStore ids
     *  @param oids a vector with the oids of the objects.
     *
     *  @return 0 on success
     */
     int list(vector<int>& oids)
     {
        return PoolSQL::list(oids, VirtualNetworkStore::table);
     }

private:
    /**
     * VirtualNetworkStore attributes to be inherited into the VM vnet
     */
    vector<string> inherit_attrs;

    /**
     *  Factory method to produce objects
     *    @return a pointer to the new object
     */
    PoolObjectSQL * create()
    {
        set<int> empty;

        return new VirtualNetworkStore(-1,-1,"","", 0, 0, empty);
    };
};

#endif /*VIRTUALNETWORKSTORE_POOL_H*/
