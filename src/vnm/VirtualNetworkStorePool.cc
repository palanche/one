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

#include "VirtualNetworkStorePool.h"
#include "Nebula.h"
#include "NebulaLog.h"

#include <stdexcept>

/* -------------------------------------------------------------------------- */
/* There is a default virtual network store boostrapped by the core :         */
/* The first 100 IDs are reserved. Regular ones start from ID 100             */
/* -------------------------------------------------------------------------- */

const string VirtualNetworkStorePool::DEFAULT_VNS_NAME = "default";
const int    VirtualNetworkStorePool::DEFAULT_VNS_ID   = 0;

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

VirtualNetworkStorePool::VirtualNetworkStorePool(
        SqlDB * db,
        const vector<const SingleAttribute *>& _inherit_attrs) :
    PoolSQL(db, VirtualNetworkStore::table, true, true)
{
    ostringstream oss;
    string        error_str;

    vector<const SingleAttribute *>::const_iterator it;

    for (it = _inherit_attrs.begin(); it != _inherit_attrs.end(); it++)
    {
        inherit_attrs.push_back((*it)->value());
    }

    //lastOID is set in PoolSQL::init_cb
    if (get_lastOID() == -1)
    {
        VirtualNetworkStoreTemplate * vns_tmpl;
        int      rc;
        set<int> cluster_ids;

        cluster_ids.insert(ClusterPool::DEFAULT_CLUSTER_ID);

        // Create the default VirtualNetworkStore
        oss.str("");

        oss << "NAME   = "   << DEFAULT_VNS_NAME << endl
            << "VN_MAD = dummy";

        vns_tmpl = new VirtualNetworkStoreTemplate;
        rc = vns_tmpl->parse_str_or_xml(oss.str(), error_str);

        if( rc < 0 )
        {
            goto error_bootstrap;
        }

        allocate(UserPool::ONEADMIN_ID,
                 GroupPool::ONEADMIN_ID,
                 UserPool::oneadmin_name,
                 GroupPool::ONEADMIN_NAME,
                 0137,
                 vns_tmpl,
                 &rc,
                 cluster_ids,
                 error_str);

        if( rc < 0 )
        {
            goto error_bootstrap;
        }

        // User created VirtualNetworkStores will start from ID 100
        set_update_lastOID(99);
    }

    return;

error_bootstrap:
    oss.str("");
    oss << "Error trying to create default VirtualNetworkStore: " << error_str;
    NebulaLog::log("VIRTUALNETWORKSTORE", Log::ERROR, oss);

    throw runtime_error(oss.str());
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualNetworkStorePool::allocate(
        int                           uid,
        int                           gid,
        const string&                 uname,
        const string&                 gname,
        int                           umask,
        VirtualNetworkStoreTemplate * vns_template,
        int *                         oid,
        const set<int>                &cluster_ids,
        string&                       error_str)
{
    VirtualNetworkStore * vns;
    VirtualNetworkStore * vns_aux = 0;

    string        name;
    ostringstream oss;

    vns = new VirtualNetworkStore(uid, gid, uname, gname, umask,
            vns_template, cluster_ids);

    vns->get_template_attribute("NAME", name);

    if ( !PoolObjectSQL::name_is_valid(name, error_str) )
    {
        goto error_name;
    }

    vns_aux = get(name, false);

    if( vns_aux != 0 )
    {
        goto error_duplicated;
    }

    *oid = PoolSQL::allocate(vns, error_str);

    return *oid;

error_duplicated:
    oss << "NAME is already taken by VIRTUALNETWORKSTORE " << vns_aux->get_oid()
    << ".";
    error_str = oss.str();

error_name:
    delete vns;
    *oid = -1;

    return *oid;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualNetworkStorePool::drop(PoolObjectSQL * objsql, string& error_msg)
{
    VirtualNetworkStore * vns = static_cast<VirtualNetworkStore*>(objsql);

    int rc;

    if( vns->virtual_networks_size() > 0 )
    {
        ostringstream oss;
        oss << "VirtualNetworkStore " << vns->get_oid() << " is not empty.";
        error_msg = oss.str();
        NebulaLog::log("VIRTUALNETWORKSTORE", Log::ERROR, error_msg);

        return -3;
    }

    rc = vns->drop(db);

    if( rc != 0 )
    {
        error_msg = "SQL DB error";
        rc = -1;
    }

    return rc;
}
