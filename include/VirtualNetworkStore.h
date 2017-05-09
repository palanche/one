/* ------------------------------------------------------------------------ */
/* Copyright 2002-2017, OpenNebula Project, OpenNebula Systems              */
/*                                                                          */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may  */
/* not use this file except in compliance with the License. You may obtain  */
/* a copy of the License at                                                 */
/*                                                                          */
/* http://www.apache.org/licenses/LICENSE-2.0                               */
/*                                                                          */
/* Unless required by applicable law or agreed to in writing, software      */
/* distributed under the License is distributed on an "AS IS" BASIS,        */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/* See the License for the specific language governing permissions and      */
/* limitations under the License.                                           */
/* -------------------------------------------------------------------------*/

#ifndef VIRTUALNETWORKSTORE_H
#define VIRTUALNETWORKSTORE_H

#include "PoolSQL.h"
#include "ObjectCollection.h"
#include "VirtualNetworkStoreTemplate.h"
#include "Clusterable.h"
#include "VirtualNetwork.h"

/**
 *  The VirtualNetworkStore class.
 */
class VirtualNetworkStore : public PoolObjectSQL, public Clusterable
{
public:
    /**
     * Function to print the VirtualNetworkStore object into a string in XML
     * format
     *  @param xml the resulting XML string
     *  @return a reference to the generated string
     */
    string& to_xml(string& xml) const;

    /**
     *  Rebuilds the object from an xml formatted string
     *    @param xml_str The xml-formatted string
     *    @return 0 on success, -1 otherwise
     */
    int from_xml(const string &xml_str);

    /**
     *  Adds this virtual network's ID to the set.
     *    @param id of the virtual network to be added to the
     *    VirtualNetworkStore
     *    @return 0 on success
     */
    int add_virtual_network(int id)
    {
        return virtual_networks.add(id);
    };

    /**
     *  Deletes this virtual network's ID from the set.
     *    @param id of the virtual network to be deleted from the
     *    VirtualNetworkStore
     *    @return 0 on success
     */
    int del_virtual_network(int id)
    {
        return virtual_networks.del(id);
    };

    /**
     *  Returns a copy of the virtual networks IDs set
     */
    set<int> get_virtual_networks_ids()
    {
        return virtual_networks.clone();
    }

    /**
     *  Returns the number of virtual networks
     */
    int virtual_networks_size()
    {
        return virtual_networks.size();
    }

    /**
     *  Retrieves vn mad name
     *    @return string vn mad name
     */
    const string& get_vn_mad() const
    {
        return vn_mad;
    };

    /**
     * Enable or disable the VirtualNetworkStore.
     * @param enable true to enable
     * @param error_str Returns the error reason, if any
     * @return 0 on success
     */
    int enable(bool enable, string& error_str);

private:

    // -------------------------------------------------------------------------
    // Friends
    // -------------------------------------------------------------------------

    friend class VirtualNetworkStorePool;

    // *************************************************************************
    // VirtualNetworkStore Private Attributes
    // *************************************************************************

    /**
     * Name of the virtual network driver used to register new virtual networks
     */
    string vn_mad;

    /**
     *  VirtualNetworkStore state
     */
    VirtualNetworkStoreState state;

    /**
     *  Collection of virtual network ids in this datastore
     */
    ObjectCollection virtual_networks;

    // *************************************************************************
    // Constructor
    // *************************************************************************

    VirtualNetworkStore(
            int                          uid,
            int                          gid,
            const string&                uname,
            const string&                gname,
            int                          umask,
            VirtualNetworkStoreTemplate* vns_template,
            const set<int>               &cluster_ids);

    virtual ~VirtualNetworkStore();

    // *************************************************************************
    // DataBase implementation (Private)
    // *************************************************************************

    static const char * db_names;

    static const char * db_bootstrap;

    static const char * table;

    /**
     *  Execute an INSERT or REPLACE Sql query.
     *    @param db The SQL DB
     *    @param replace Execute an INSERT or a REPLACE
     *    @param error_str Returns the error reason, if any
     *    @return 0 one success
     */
    int insert_replace(SqlDB *db, bool replace, string& error_str);

    /**
     *  Bootstraps the database table(s) associated to the VirtualNetworkStore
     *    @return 0 on success
     */
    static int bootstrap(SqlDB * db)
    {
        ostringstream oss(VirtualNetworkStore::db_bootstrap);

        return db->exec(oss);
    };

    /**
     *  Writes the VirtualNetworkStore in the database.
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int insert(SqlDB *db, string& error_str);

    /**
     *  Writes/updates the VirtualNetworkStore's data fields in the database.
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int update(SqlDB *db)
    {
        string error_str;
        return insert_replace(db, true, error_str);
    }

    /**
     *  Factory method for datastore templates
     */
    Template * get_new_template() const
    {
        return new VirtualNetworkStoreTemplate;
    }

    /**
     *  Verify the proper definition of the VN_MAD by checking the attributes
     *  related to the VN defined in VN_MAD_CONF specified in the
     *  VirtualNetworkStore template
     */
    int set_vn_mad(string &vn_mad, string &error_str);

    /**
     * Child classes can process the new template set with replace_template or
     * append_template with this method
     *    @param error string describing the error if any
     *    @return 0 on success
     */
    int post_update_template(string& error);
};

#endif /*VIRTUALNETWORKSTORE_H*/
