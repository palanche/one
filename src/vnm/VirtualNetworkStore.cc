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
/* ------------------------------------------------------------------------ */

#include "VirtualNetworkStore.h"
#include "GroupPool.h"
#include "NebulaLog.h"
#include "Nebula.h"

const char * VirtualNetworkStore::table = "virtualnetworkstore_pool";

const char * VirtualNetworkStore::db_names =
        "oid, name, body, uid, gid, owner_u, group_u, other_u";

const char * VirtualNetworkStore::db_bootstrap =
    "CREATE TABLE IF NOT EXISTS virtualnetworkstore_pool ("
    "oid INTEGER PRIMARY KEY, name VARCHAR(128), body MEDIUMTEXT, uid INTEGER, "
    "gid INTEGER, owner_u INTEGER, group_u INTEGER, other_u INTEGER)";

/* ************************************************************************ */
/* VirtualNetworkStore :: Constructor/Destructor                            */
/* ************************************************************************ */

VirtualNetworkStore::VirtualNetworkStore(
        int                          uid,
        int                          gid,
        const string&                uname,
        const string&                gname,
        int                          umask,
        VirtualNetworkStoreTemplate* vns_template,
        const set<int>               &cluster_ids):
            PoolObjectSQL(-1,VIRTUALNETWORKSTORE,"",uid,gid,uname,gname,table),
            Clusterable(cluster_ids),
            vn_mad(""),
            state(READY),
            virtual_networks("VIRTUALNETWORKS")
{
    if (vns_template != 0)
    {
        obj_template = vns_template;
    }
    else
    {
        obj_template = new VirtualNetworkStoreTemplate;
    }

    set_umask(umask);

    group_u = 1;
}

VirtualNetworkStore::~VirtualNetworkStore()
{
    delete obj_template;
};

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

int VirtualNetworkStore::enable(bool enable, string& error_str)
{
    if (enable)
    {
        state = READY;
    }
    else
    {
        state = DISABLED;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

int VirtualNetworkStore::set_vn_mad(std::string &mad, std::string &error_str)
{
    const VectorAttribute* vatt;
    int rc;
    std::vector<std::string> vrequired_attrs;
    std::string              required_attrs, required_attr, value;
    std::ostringstream       oss;

    rc = Nebula::instance().get_vn_conf_attribute(mad, vatt);

    if ( rc != 0 )
    {
        goto error_conf;
    }

    rc = vatt->vector_value("REQUIRED_ATTRS", required_attrs);

    if ( rc == -1 ) //No required attributes
    {
        return 0;
    }

    vrequired_attrs = one_util::split(required_attrs, ',');

    for ( std::vector<std::string>::const_iterator it = vrequired_attrs.begin();
         it != vrequired_attrs.end(); it++ )
    {
        required_attr = *it;

        required_attr = one_util::trim(required_attr);
        one_util::toupper(required_attr);

        get_template_attribute(required_attr.c_str(), value);

        if ( value.empty() )
        {
            goto error_required;
        }
    }

    return 0;

error_conf:
    oss << "VN_MAD named \"" << mad << "\" is not defined in oned.conf";
    goto error_common;

error_required:
    oss << "VirtualNetworkStore template is missing the \"" << required_attr
        << "\" attribute or it's empty.";

error_common:
    error_str = oss.str();
    return -1;
}

/* -------------------------------------------------------------------------- */

int VirtualNetworkStore::insert(SqlDB *db, string& error_str)
{
    ostringstream oss;

    // VirtualNetworkStorePool::allocate checks NAME
    erase_template_attribute("NAME", name);

    get_template_attribute("VN_MAD", vn_mad);

    if ( vn_mad.empty() == true )
    {
        goto error_empty_vn;
    }

    if (set_vn_mad(vn_mad, error_str) != 0)
    {
        goto error_common;
    }

    // Insert the VirtualNetworkStore
    return insert_replace(db, false, error_str);

error_empty_vn:
    error_str = "No VN_MAD in template.";
    goto error_common;

error_common:
    NebulaLog::log("VIRTUALNETWORKSTORE", Log::ERROR, error_str);
    return -1;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

int VirtualNetworkStore::insert_replace(SqlDB *db, bool replace, string& error_str)
{
    ostringstream   oss;

    int    rc;
    string xml_body;

    char * sql_name;
    char * sql_xml;

    // Update the VirtualNetworkStore
    sql_name = db->escape_str(name.c_str());

    if ( sql_name == 0 )
    {
        goto error_name;
    }

    sql_xml = db->escape_str(to_xml(xml_body).c_str());

    if ( sql_xml == 0 )
    {
        goto error_body;
    }

    if ( validate_xml(sql_xml) != 0 )
    {
        goto error_xml;
    }

    if ( replace )
    {
        oss << "REPLACE";
    }
    else
    {
        oss << "INSERT";
    }

    // Construct the SQL statement to Insert or Replace
    oss <<" INTO "<<table <<" ("<< db_names <<") VALUES ("
        <<          oid                 << ","
        << "'" <<   sql_name            << "',"
        << "'" <<   sql_xml             << "',"
        <<          uid                 << ","
        <<          gid                 << ","
        <<          owner_u             << ","
        <<          group_u             << ","
        <<          other_u             << ")";

    rc = db->exec(oss);

    db->free_str(sql_name);
    db->free_str(sql_xml);

    return rc;

error_xml:
    db->free_str(sql_name);
    db->free_str(sql_xml);

    error_str = "Error transforming the VirtualNetworkStore to XML.";

    goto error_common;

error_body:
    db->free_str(sql_name);
    goto error_generic;

error_name:
    goto error_generic;

error_generic:
    error_str = "Error inserting VirtualNetworkStore in DB.";
error_common:
    return -1;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

string& VirtualNetworkStore::to_xml(string& xml) const
{
    ostringstream   oss;
    string          clusters_xml;
    string          virtual_networks_xml;
    string          template_xml;
    string          perms_xml;

    oss <<
    "<VIRTUALNETWORKSTORE>"     <<
        "<ID>"                  << oid          << "</ID>"        <<
        "<UID>"                 << uid          << "</UID>"       <<
        "<GID>"                 << gid          << "</GID>"       <<
        "<UNAME>"               << uname        << "</UNAME>"     <<
        "<GNAME>"               << gname        << "</GNAME>"     <<
        "<NAME>"                << name         << "</NAME>"      <<
        perms_to_xml(perms_xml) <<
        "<VN_MAD>"    << one_util::escape_xml(vn_mad)    << "</VN_MAD>" <<
        "<STATE>"     << state                           << "</STATE>"  <<
        Clusterable::to_xml(clusters_xml)                <<
        virtual_networks.to_xml(virtual_networks_xml)    <<
        obj_template->to_xml(template_xml)               <<
    "</VIRTUALNETWORKSTORE>";

    xml = oss.str();

    return xml;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

int VirtualNetworkStore::from_xml(const string& xml)
{
    int rc = 0;
    int int_state;
    vector<xmlNodePtr> content;

    ostringstream oss;

    // Initialize the internal XML object
    update_from_str(xml);

    // Get class base attributes
    rc += xpath(oid,          "/VIRTUALNETWORKSTORE/ID",        -1);
    rc += xpath(uid,          "/VIRTUALNETWORKSTORE/UID",       -1);
    rc += xpath(gid,          "/VIRTUALNETWORKSTORE/GID",       -1);
    rc += xpath(uname,        "/VIRTUALNETWORKSTORE/UNAME",     "not_found");
    rc += xpath(gname,        "/VIRTUALNETWORKSTORE/GNAME",     "not_found");
    rc += xpath(name,         "/VIRTUALNETWORKSTORE/NAME",      "not_found");
    rc += xpath(vn_mad,       "/VIRTUALNETWORKSTORE/VN_MAD",    "not_found");
    rc += xpath(int_state,    "/VIRTUALNETWORKSTORE/STATE",     0);

    // Permissions
    rc += perms_from_xml();

    state = static_cast<VirtualNetworkStoreState>(int_state);

    // Set of virtual networks IDs
    rc += virtual_networks.from_xml(this, "/VIRTUALNETWORKSTORE/");

    // Set of cluster IDs
    rc += Clusterable::from_xml(this, "/VIRTUALNETWORKSTORE/");

    // Get associated classes
    ObjectXML::get_nodes("/VIRTUALNETWORKSTORE/TEMPLATE", content);

    if (content.empty())
    {
        return -1;
    }

    rc += obj_template->from_xml_node(content[0]);

    ObjectXML::free_nodes(content);

    if (rc != 0)
    {
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

int VirtualNetworkStore::post_update_template(string& error_str)
{
    string new_vn_mad;
    string old_vn_mad = vn_mad;

    /* ---------------------------------------------------------------------- */
    /* Set the VN_MAD of the VirtualNetworkStore (class & template)           */
    /* ---------------------------------------------------------------------- */
    get_template_attribute("VN_MAD", new_vn_mad);

    if ( new_vn_mad.empty() )
    {
        replace_template_attribute("VN_MAD", vn_mad);
    }
    else if ( new_vn_mad != vn_mad)
    {
        vn_mad = new_vn_mad;
    }

    /* ---------------------------------------------------------------------- */
    /* Verify that the template has the required attributees                  */
    /* ---------------------------------------------------------------------- */
    if ( set_vn_mad(vn_mad, error_str) !=  0 )
    {
        vn_mad    = old_vn_mad;

        return -1;
    }

    return 0;
}
