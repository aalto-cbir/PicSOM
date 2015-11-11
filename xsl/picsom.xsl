<?xml version="1.0" encoding="utf-8"?>

<!-- 
// -*- C++ -*-  $Id: picsom.xsl,v 1.78 2015/04/21 09:31:29 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 
-->

<xsl:stylesheet 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:fo="http://www.w3.org/1999/XSL/Format"
  xmlns:picsom="http://picsom.ics.aalto.fi/picsom/ns"
  version="1.0">

  <xsl:param name="tnwidth"   select="180"/>
  <xsl:param name="tnheight"  select="135"/>
  <xsl:param name="tncolumns" select="5"/>
  <xsl:param name="videosegm">bb-25-05</xsl:param>

  <xsl:variable name="useradioselection" select="0"/>

  <xsl:output method="html" indent="yes"/>

  <!-- 
     <xsl:output method="xml" omit-xml-declaration="yes" indent="yes"
		 media-type="text/html"/> 
  -->

  <xsl:template match="/">
    <html>
      <head>
        <xsl:call-template name="meta"/>
        <title>PicSOM content-based multimedia retrieval</title>
        <script src="/picsom.js" type="text/javascript"/>
      </head>
      <body>
        <h1>PicSOM content-based multimedia retrieval</h1>
        <!-- <pre id="xmlstuff">(nothing yet)</pre> -->
        <xsl:apply-templates/>
      </body>
    </html>
  </xsl:template>

  <xsl:template name="meta">
    <xsl:copy-of select="//meta"/>
    <meta name="DC.Title" content="PicSOM content-based image retrieval"/>
    <meta name="DC.Subject" content="content-based image retrieval"/>
    <meta name="AC.Email" content="picsom@ics.aalto.fi"/>
    <xsl:for-each select="//picsom:result/picsom:refresh">
      <meta http-equiv="refresh">
        <xsl:attribute name="content"><xsl:value-of select="."/></xsl:attribute>
      </meta>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="//picsom:linkxxx">
    <xsl:call-template name="link"/>
    <br/>
  </xsl:template>

  <xsl:template name="link">
    <xsl:element name="a">
      <xsl:attribute name="href"><xsl:value-of select="@href"/></xsl:attribute>
      <xsl:value-of select="@linktext"/>
    </xsl:element><xsl:text>  </xsl:text>
    <xsl:value-of select="@text"/>
  </xsl:template>

  <xsl:template name="infolinkrow">
    <tr>
      <td>
	<xsl:element name="a">
	  <xsl:attribute name="href"><xsl:value-of select="@href"/></xsl:attribute>
	  <xsl:value-of select="@href"/>
	</xsl:element>
      </td>
      <td>
	<xsl:value-of select="@linktext"/>
      </td>
      <td>
	<xsl:value-of select="@text"/>
      </td>
      <td>
	<xsl:element name="a">
	  <xsl:attribute name="href"><xsl:value-of select="@imgsrc"/></xsl:attribute>
	  <xsl:value-of select="@imgsrc"/>
	</xsl:element>
      </td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:infolinklist" name="infolinklist">
    <table border="1">
      <thead style="font-weight:bold">
	<tr>
	  <td>href</td><td>linktext</td><td>text</td><td>imgsrc</td>
	</tr>
      </thead>
      <tbody>
	<xsl:for-each select="picsom:infolink">
	  <xsl:call-template name="infolinkrow"/>
	</xsl:for-each>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template name="visitedlinkrow">
    <tr>
      <td>
	<xsl:element name="a">
	  <xsl:attribute name="href"><xsl:value-of select="@url"/></xsl:attribute>
	  <xsl:value-of select="@url"/>
	</xsl:element>
      </td>
      <td>
	<xsl:value-of select="@content-type"/>
      </td>
      <td>
	<xsl:value-of select="@size"/>
      </td>
      <td>
	<xsl:value-of select="@time"/>
      </td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:visitedlinklist" name="visitedlinklist">
    <table border="1">
      <thead style="font-weight:bold">
	<tr>
	  <td>url</td><td>content-type</td><td>size</td><td>time</td>
	</tr>
      </thead>
      <tbody>
	<xsl:for-each select="picsom:visitedlink">
	  <xsl:call-template name="visitedlinkrow"/>
	</xsl:for-each>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template name="image">
    <xsl:element name="img">
      <xsl:attribute name="src"><xsl:value-of select="@src"/></xsl:attribute>
      <xsl:attribute name="width">640</xsl:attribute>
      <xsl:attribute name="height">360</xsl:attribute>
    </xsl:element>
  </xsl:template>

  <xsl:template match="//picsom:get-databaselist">
    <xsl:element name="a">
      <xsl:attribute name="href">/databaselist</xsl:attribute>
      databaselist
    </xsl:element>
    <xsl:element name="br"/>
  </xsl:template>

  <xsl:template match="//picsom:get-querylist">
    <xsl:element name="a">
      <xsl:attribute name="href">/querylist</xsl:attribute>
      querylist
    </xsl:element>
    <xsl:element name="br"/>
  </xsl:template>

  <xsl:template match="//picsom:get-versions">
    <xsl:element name="a">
      <xsl:attribute name="href">/versions</xsl:attribute>
      versions
    </xsl:element>
    <xsl:element name="br"/>
  </xsl:template>

  <xsl:template match="//picsom:slavelist">
    <h2> List of existing slaves </h2>
    <table border="1">
      <thead style="font-weight:bold">
        <tr>
          <td>hostspec</td>
          <td>hostname</td>
          <td>executable</td>
          <td>status</td>
          <td>load</td>
          <td>cpucount</td>
          <td>cpuusage</td>
          <td>submitted</td>
          <td>started</td>
          <td>updated</td>
          <td>spare</td>
          <td>conn</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:slave">
    <tr>
    <td><xsl:value-of select="picsom:hostspec"/></td>
    <td><xsl:value-of select="picsom:hostname"/></td>
    <td><xsl:value-of select="picsom:executable"/></td>
    <td><xsl:value-of select="picsom:status"/></td>
    <td><xsl:value-of select="picsom:load"/></td>
    <td><xsl:value-of select="picsom:cpucount"/></td>
    <td><xsl:value-of select="picsom:cpuusage"/></td>
    <td><xsl:value-of select="picsom:submitted"/></td>
    <td><xsl:value-of select="picsom:started"/></td>
    <td><xsl:value-of select="picsom:updated"/></td>
    <td><xsl:value-of select="picsom:spare"/></td>
    <td><xsl:value-of select="picsom:conn"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:threadlist">
    <h2> List of existing threads </h2>
    <table border="1">
      <thead style="font-weight:bold">
        <tr>
          <td>name</td>
          <td>pthread</td>
          <td>phase</td>
          <td>type</td>
          <td>text</td>
          <td>state</td>
          <td>slave_state</td>
          <td>showname</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="picsom:thread">
    <tr>
      <xsl:call-template name="threadinner"/>
    </tr>
  </xsl:template>

  <xsl:template name="threadinner">
    <td><xsl:value-of select="picsom:name"/></td>
    <td><xsl:value-of select="picsom:pthread"/></td>
    <td><xsl:value-of select="picsom:phase"/></td>
    <td><xsl:value-of select="picsom:type"/></td>
    <td><xsl:value-of select="picsom:text"/></td>
    <td><xsl:value-of select="picsom:state"/></td>
    <td><xsl:value-of select="picsom:slave_state"/></td>
  </xsl:template>

  <xsl:template match="//picsom:analysislist">
    <h2> List of existing analyses </h2>
    <table>
      <thead style="font-weight:bold">
        <tr>
          <td>identity</td>
          <td>memory</td>
          <td>name</td>
          <td>pthread</td>
          <td>phase</td>
          <td>type</td>
          <td>text</td>
          <td>state</td>
          <td>slave_state</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:analysis">
    <tr>
      <td><xsl:value-of select="picsom:identity"/></td>
      <td><xsl:value-of select="picsom:memory"/></td>
      <xsl:for-each select="picsom:thread">
        <xsl:call-template name="threadinner"/>
      </xsl:for-each>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:databaselist">
    <h2> List of available databases </h2>
    <table border="1">
      <thead style="font-weight:bold">
        <tr>
          <td>name</td>
          <td>long name</td>
          <td>description</td>
          <td>size</td>
          <td>objectypes</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:databaselist/picsom:database">
    <tr>
      <td>
        <xsl:element name="a">
          <xsl:attribute name="href">/database/<xsl:value-of
						  select="picsom:name"/>
          </xsl:attribute>
          <xsl:value-of select="picsom:name"/>
        </xsl:element>
      </td>
      <td><xsl:value-of select="picsom:longname"/></td>
      <td>
        <span><xsl:value-of select="picsom:shorttext"/></span>
        <span style="display:none"><xsl:value-of select="picsom:longtext"/>
        </span>
        <span>
          <button type="button" onclick='toggle_short_long(this)'>MORE</button>
        </span>
        <xsl:variable name="context" select="picsom:context"/>
        <xsl:if test="$context='yes'"><xsl:element name="a">
        <xsl:attribute name="href"><xsl:value-of 
        select="picsom:name"/>/contextstate</xsl:attribute>
        contextstate</xsl:element></xsl:if>
      </td><xsl:text>
  </xsl:text>
      <td><xsl:value-of select="picsom:size"/></td>
      <td><xsl:value-of select="picsom:objecttypes"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:result/picsom:database">
    Database: <xsl:apply-templates select="picsom:longname"/><br/>
    Query target: <xsl:apply-templates
		     select="//picsom:variables/picsom:target"/><br/>
    Text search: <xsl:apply-templates
		     select="//picsom:variables/picsom:textsearch"/><br/>
    Text index: <xsl:apply-templates
		     select="//picsom:variables/picsom:textindex"/><br/>
    Text field: <xsl:apply-templates
		     select="//picsom:variables/picsom:textfield"/><br/>
    Text query: <xsl:apply-templates
		     select="//picsom:variables/picsom:textquery"/>
  </xsl:template>
  
  <xsl:template match="//picsom:querylist">
    <h2> List of existing queries </h2>
    <table border="1">
      <thead style="font-weight:bold">
        <tr>
          <td>identity</td>
          <td>database</td>
          <td>client</td>
          <td>starttime</td>
        </tr>
      </thead>
      <tbody>
	<xsl:for-each select="picsom:queryinfo">
	  <xsl:call-template name="queryinforow"/>
	</xsl:for-each>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template name="queryinforow">
    <tr>
      <td>
        <xsl:element name="a">
          <xsl:attribute name="href">/query/<xsl:value-of
						  select="picsom:identity"/>/</xsl:attribute>
          <xsl:value-of select="picsom:identity"/>
        </xsl:element>
      </td>
      <td><xsl:value-of select="picsom:databasename"/></td>
      <td><xsl:value-of select="picsom:client"/></td>
      <td><xsl:value-of select="picsom:starttime"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:queryinfo">
    <h2> Info on query <xsl:value-of select="picsom:identity"/></h2>
    <table border="0">
      <tbody>
	<tr>
	  <td><b>identity</b></td>
	  <td><xsl:value-of select="picsom:identity"/></td>
	</tr>
	<tr>
	  <td><b>databasename</b></td>
	  <td><xsl:value-of select="picsom:databasename"/></td>
	</tr>
	<tr>
	  <td><b>client</b></td>
	  <td><xsl:value-of select="picsom:client"/></td>
	</tr>
	<tr>
	  <td><b>starttime</b></td>
	  <td><xsl:value-of select="picsom:starttime"/></td>
	</tr>
	<tr>
	  <td><b>modtime</b></td>
	  <td><xsl:value-of select="picsom:modtime"/></td>
	</tr>
	<tr>
	  <td><b>accesstime</b></td>
	  <td><xsl:value-of select="picsom:accesstime"/></td>
	</tr>
	<tr>
	  <td><b>savetime</b></td>
	  <td><xsl:value-of select="picsom:savetime"/></td>
	</tr>
	<tr>
	  <td><b>needs_save</b></td>
	  <td><xsl:value-of select="picsom:needs_save"/></td>
	</tr>
	<tr>
	  <td><b>from_disk</b></td>
	  <td><xsl:value-of select="picsom:from_disk"/></td>
	</tr>
	<tr>
	  <td><b>nodes</b></td>
	  <td><xsl:value-of select="picsom:nodes"/></td>
	</tr>
	<tr>
	  <td><b>leaves</b></td>
	  <td><xsl:value-of select="picsom:leaves"/></td>
	</tr>
	<tr>
	  <td><b>imagecount</b></td>
	  <td><xsl:value-of select="picsom:imagecount"/></td>
	</tr>
       </tbody>
     </table>
     <br/>
     <xsl:for-each select="picsom:infolinklist">
       <xsl:call-template name="infolinklist"/>
     </xsl:for-each>
     <br/>
     <xsl:for-each select="picsom:visitedlinklist">
       <xsl:call-template name="visitedlinklist"/>
     </xsl:for-each>
     <br/>
     <xsl:for-each select="picsom:image">
       <xsl:call-template name="image"/>
       <br/>
     </xsl:for-each>
  </xsl:template>

  <xsl:template match="//picsom:featurelist">
    <span id="featurelist"> 
      <form>
	<xsl:attribute name="method">post</xsl:attribute>
	<xsl:attribute name="action">/action</xsl:attribute>
	<xsl:attribute name="enctype">multipart/form-data</xsl:attribute>
	<xsl:attribute name="name">f</xsl:attribute>
	<xsl:attribute name="id">f_id</xsl:attribute>
	<p/>
	<input type="submit" name="start" value="Start query"/>
	<button type="button" name="restart" 
		onclick="window.location.href='/'">Restart</button><xsl:text>
	</xsl:text>
	<xsl:call-template name="algorithmlistinner"/>
	<xsl:call-template name="featurelistinner"/>
	<xsl:call-template name="contentitemlistinner"/>
	<xsl:call-template name="textqueryboxes"/>
	<p/>
	<input type="submit" name="start" value="Start query"/>
	<button type="button" name="restart" 
		onclick="window.location.href='/'">Restart</button><xsl:text>
	</xsl:text>
      </form>
    </span>
    <xsl:call-template name="tssomsurfaces"/>
  </xsl:template>

  <xsl:template name="featurelistinner">
    <h2> List of available features </h2>
    <xsl:for-each select="//picsom:featurelist">
      <xsl:copy/>
      <table>
        <thead style="font-weight:bold">
          <tr>
            <td></td>
            <td>longname</td>
            <td>shorttext</td>
            <td>method</td>
            <td>size</td>
            <td>objecttype</td>
            <td>counts</td>
            <td>type</td>
          </tr>
        </thead>
        <xsl:apply-templates select="picsom:feature">
          <xsl:sort select="picsom:name"/>
        </xsl:apply-templates>
      </table>
    </xsl:for-each>
    <xsl:apply-templates select="picsom:featurealias"/>
  </xsl:template>

  <xsl:template match="//picsom:feature">
    <tr>
      <td>
        <input>
          <xsl:attribute name="type">checkbox</xsl:attribute>
          <xsl:attribute name="name">featsel</xsl:attribute>
          <xsl:attribute name="value"><xsl:value-of select="picsom:name"/>
          </xsl:attribute>
          <xsl:variable name="selected" select="picsom:selected"/>
          <xsl:if test="$selected=1"><xsl:attribute name="checked"/></xsl:if>
          <xsl:attribute name="id"><xsl:value-of select="picsom:name"/>
          </xsl:attribute>
        </input>
      </td>
      <td><xsl:value-of select="picsom:longname"/></td>
      <td><xsl:value-of select="picsom:shorttext"/></td>
      <td><xsl:value-of select="picsom:method"/></td>
      <td>
        <xsl:variable name="fsize" select="picsom:size"/>
        <xsl:choose>
          <xsl:when test="$fsize > 0"><xsl:value-of select="$fsize"/></xsl:when>
          <xsl:otherwise>-</xsl:otherwise>
        </xsl:choose>
      </td>
      <td><xsl:value-of select="picsom:objecttype"/></td>
      <td><xsl:value-of select="picsom:counts"/></td>
      <td><xsl:value-of select="picsom:type"/></td>
    </tr><xsl:text>
    </xsl:text>
  </xsl:template>

  <xsl:template match="//picsom:featurealias">
    <xsl:variable name="faname" select="@name"/>
    <xsl:variable name="fatext" select="@text"/>
    <xsl:variable name="favalue" select="@value"/>
    <button type="button" name="{$faname}" onclick='set_features("{$favalue}")'>
      <xsl:value-of select="$fatext"/></button><xsl:text>
    </xsl:text>
  </xsl:template>

  <xsl:template name="contentitemlistrow">
    <tr>
      <td>
    <input>
      <xsl:attribute name="type">checkbox</xsl:attribute>
      <xsl:attribute name="name">classaugment</xsl:attribute>

      <xsl:attribute name="value">
        <xsl:value-of select="picsom:name"/>
      </xsl:attribute>
      <xsl:attribute name="id">
        <xsl:value-of select="picsom:name"/>
      </xsl:attribute>

      <xsl:variable name="myname" select="picsom:name"/>
      <xsl:variable name="selected"
        select="//picsom:classaugment/picsom:class[@name=$myname]/@value"/>
      <xsl:if test="$selected=1"><xsl:attribute name="checked"/></xsl:if>
      
    </input>
	<xsl:variable name="cname" select="picsom:name"/>
	<xsl:choose>
	  <xsl:when test="contains($cname, 'classdef::')">
	    <xsl:value-of select="substring-after($cname, 'classdef::')"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="$cname"/>
	  </xsl:otherwise>
	</xsl:choose>
      </td>
      <td align="right"><xsl:value-of select="picsom:true"/></td>
      <td align="right"><xsl:value-of select="picsom:false"/></td>
      <td align="right"><xsl:value-of select="picsom:apriori"/></td>
      <td align="right"><xsl:value-of select="picsom:cover"/></td>
      <td><xsl:value-of select="picsom:description"/></td>
    </tr>
  </xsl:template>

  <xsl:template name="contentitemlistinner">
      <h2> List of existing content classes </h2>
      <table>
	<thead style="font-weight:bold">
          <tr>
            <td>name</td>
            <td>positives</td>
            <td>negatives</td>
            <td>a priori</td>
            <td>cover</td>
            <td>description</td>
          </tr>
	</thead>
	<tbody>
	  <xsl:for-each select="//picsom:contentitem">
	    <xsl:sort select="picsom:true" order="descending" data-type="number"/>
            <xsl:call-template name="contentitemlistrow"/>
	  </xsl:for-each>
	</tbody>
      </table>
  </xsl:template>

  <xsl:template name="textqueryboxes">
    <span id="textquerysection" style="display:"> 
      <xsl:variable name="allowtextquery" 
                    select="//picsom:database/picsom:allowtextquery"/>
      <xsl:if test="$allowtextquery='yes'">
        <xsl:call-template name="textquerytemplate"/>
        <xsl:call-template name="textsearchresulttemplate"/>
      </xsl:if>
    </span>
  </xsl:template>

  <xsl:template name="tssomsurfaces">
    <span id="tssomsurfaces"> 
      <xsl:variable name="hasquests"
		    select="//picsom:questionobjectlist//picsom:objectinfo"/>
      <xsl:if test="$hasquests">
	<h2> TS-SOM surfaces  </h2>
	<xsl:for-each select="//picsom:featurelist">
	  <table>
	    <thead align="center" style="font-weight:bold">
	      <tr>
		<xsl:for-each select="picsom:feature">
		  <xsl:variable name="selected" select="picsom:selected"/>
		  <xsl:variable name="method"   select="picsom:method"/>
		  <xsl:if test="($selected=1) and ($method='tssom')">
		    <td>
		      <xsl:value-of select="picsom:longname"/>
		    </td>
		  </xsl:if>
		</xsl:for-each>
	      </tr>
	    </thead>
	    <tbody align="center">
	      <tr>
		<xsl:for-each select="picsom:feature">
		  <xsl:variable name="selected" select="picsom:selected"/>
		  <xsl:variable name="method"   select="picsom:method"/>
		  <xsl:if test="($selected=1) and ($method='tssom')">
		    <td>
		      <xsl:variable name="slevel" select="picsom:showlevels"/>
		      <xsl:element name="img">
			<xsl:attribute name="src">image/tssom/<xsl:value-of 
			   select="picsom:name"/>[<xsl:value-of 
                                   select="picsom:showlevels"/>]/convolved
			</xsl:attribute>
			<xsl:attribute name="border">2</xsl:attribute>
			<xsl:attribute name="width">125</xsl:attribute>
		      </xsl:element> 
		    </td>
		  </xsl:if>
		</xsl:for-each>
	      </tr>
	    </tbody>
	  </table>
	</xsl:for-each>
      </xsl:if>
    </span>
  </xsl:template>

  <xsl:template name="textsearchresulttemplate">
    <h2> Text search results </h2>
    <table>
      <xsl:for-each
	 select="//picsom:textsearchresultobjectlist/picsom:objectinfo">
	<tr>
	  <td><xsl:value-of select="picsom:label"/></td>
	  <td><xsl:value-of select="picsom:score"/></td>
	  <td><xsl:value-of select="picsom:snippet"/></td>
	</tr>
      </xsl:for-each>
    </table>
  </xsl:template>

  <xsl:template name="textquerytemplate">
    <!-- text query -->
    <h2> Text query </h2>
    <input type="text" name="textquery"
           value="{//picsom:variables/picsom:textquery}" />
    <p/>
    
    <h3> Implicit text query: </h3>
    <xsl:value-of select="//picsom:variables/picsom:implicit_textquery"/>

    <xsl:if test="//picsom:contentitemlist">
      <!-- concept selection -->
      <h3> Activate classes (augment SOMs): </h3>
      <xsl:for-each select="//picsom:contentitemlist">
        <xsl:apply-templates select="picsom:contentitem">
          <xsl:sort select="picsom:name"/>
        </xsl:apply-templates>
      </xsl:for-each>
    </xsl:if>

    <!-- precalculated scores -->
    <h3> Activate precalculated scores: </h3>
    <xsl:for-each select="//picsom:featurelist//picsom:feature">
      <xsl:variable name="fmethod" select="picsom:method"/>
      <xsl:if test="$fmethod='precalculated'">
        <xsl:call-template name="precalculatedscores"/>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="contentdisplaytemplate">
    <xsl:if test="//picsom:contentitemlist">
      <!-- concept display, not selection -->
	<h3> Active content classes: </h3>
	<xsl:for-each select="//picsom:contentitemlist">
	  <xsl:sort select="picsom:true" order="descending" data-type="number"/>
	  <xsl:call-template name="contentitemdisplay"/>
	</xsl:for-each>
    </xsl:if>
  </xsl:template>

  <xsl:template name="precalculatedscores">
    <input>
      <xsl:attribute name="type">checkbox</xsl:attribute>
      <xsl:attribute name="name">featsel</xsl:attribute>

      <xsl:attribute name="value">
        <xsl:value-of select="picsom:name"/>
      </xsl:attribute>
      <xsl:attribute name="id">
        <xsl:value-of select="picsom:name"/>
      </xsl:attribute>

      <xsl:variable name="myname" select="picsom:name"/>
      <xsl:variable name="selected" select="picsom:selected"/>
      <xsl:if test="$selected=1"><xsl:attribute name="checked"/></xsl:if>
      
    </input>
    <label for="{picsom:name}">
      <span style="white-space:nowrap"><xsl:value-of select="picsom:name"/></span>
    </label>
    <xsl:text>&#160;&#160;</xsl:text>
  </xsl:template>

  <xsl:template match="picsom:contentitem">
    <input>
      <xsl:attribute name="type">checkbox</xsl:attribute>
      <xsl:attribute name="name">classaugment</xsl:attribute>

      <xsl:attribute name="value">
        <xsl:value-of select="picsom:name"/>
      </xsl:attribute>
      <xsl:attribute name="id">
        <xsl:value-of select="picsom:name"/>
      </xsl:attribute>

      <xsl:variable name="myname" select="picsom:name"/>
      <xsl:variable name="selected"
        select="//picsom:classaugment/picsom:class[@name=$myname]/@value"/>
      <xsl:if test="$selected=1"><xsl:attribute name="checked"/></xsl:if>
      
    </input>
    <label for="{picsom:name}">
      <span style="white-space:nowrap"><xsl:value-of select="picsom:name"/></span>
    </label>
    <xsl:text>&#160;&#160;</xsl:text>
  </xsl:template>

  <xsl:template name="contentitemdisplay">
    <xsl:variable name="myname" select="picsom:name"/>
    <xsl:variable name="selected"
		  select="//picsom:classaugment/picsom:class[@name=$myname]/@value"/>
    <xsl:if test="$selected=1">
      
    <label for="{picsom:name}">
      <span style="white-space:nowrap">
	<xsl:variable name="cname" select="picsom:name"/>
	<xsl:choose>
	  <xsl:when test="contains($cname, 'classdef::')">
	    <xsl:value-of select="substring-after($cname, 'classdef::')"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="$cname"/>
	  </xsl:otherwise>
	</xsl:choose>
	(<xsl:value-of select="picsom:true"/>) 
      </span>
    </label>
    <xsl:text>&#160;&#160;</xsl:text>

    </xsl:if>
  </xsl:template>

  <xsl:template match="//picsom:algorithmlist"/>

  <xsl:template match="//picsom:algorithmlistXX">
    <span id="algorithmlist"> 
    <h2> List of available algorithms </h2>
    <form>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="action">/action</xsl:attribute>
      <xsl:attribute name="enctype">multipart/form-data</xsl:attribute>
      <xsl:attribute name="name">f</xsl:attribute>
      <xsl:attribute name="id">f_id</xsl:attribute>
      <table>
        <xsl:apply-templates/>
      </table>
      <input type="submit" name="start" value="Start query"/>
    </form>
  </span>
  </xsl:template>

  <xsl:template name="algorithmlistinner">
    <h2> List of available algorithms </h2>
    <xsl:for-each select="//picsom:algorithmlist">
      <xsl:copy/>
      <table>
        <thead style="font-weight:bold">
          <tr>
            <td></td>
            <td>name</td>
            <td>label</td>
            <td>description</td>
          </tr>
        </thead>
        <xsl:apply-templates/>
      </table>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="//picsom:algorithm">
    <tr>
      <td>
        <input>
          <xsl:attribute name="type">radio</xsl:attribute>
          <xsl:attribute name="name">algsel</xsl:attribute>
          <xsl:attribute name="value"><xsl:value-of select="picsom:name"/>
          </xsl:attribute>
          <xsl:variable name="default" select="picsom:default"/>
          <xsl:if test="$default=1"><xsl:attribute name="checked"/></xsl:if>
          <xsl:attribute name="id"><xsl:value-of select="picsom:name"/>
          </xsl:attribute>
          <xsl:attribute name="onclick">
            javascript:toggleInput('<xsl:value-of select="picsom:name"/>')
          </xsl:attribute>
        </input>
      </td>
      <td><xsl:value-of select="picsom:name"/></td>
      <td><xsl:value-of select="picsom:label"/></td>
      <td><xsl:value-of select="picsom:description"/></td>
    </tr>
    <xsl:apply-templates select="picsom:parameterlist"/>
  </xsl:template>

  <xsl:template match="//picsom:parameterlist">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="//picsom:parameter">
    <tr>
      <td/>
      <td colspan="2">
        <xsl:if test="@type='xsd:boolean'">
          <input>
            <xsl:attribute name="type">checkbox</xsl:attribute>
            <xsl:attribute name="name">paramsel::<xsl:value-of 
              select="../../picsom:name"/></xsl:attribute>
            <xsl:attribute name="value"><xsl:value-of select="@name"/>
            </xsl:attribute>
            <xsl:if test="@defval='true'">
              <xsl:attribute name="checked"/>
            </xsl:if>
            <xsl:attribute name="disabled">true</xsl:attribute>
          </input>
          <xsl:value-of select="@name"/>
        </xsl:if>
        <xsl:if test="@type='xsd:token'">
          <xsl:variable name="radioname"><xsl:value-of 
            select="../../picsom:name"/>:<xsl:value-of 
            select="@name"/></xsl:variable>
          <xsl:value-of select="@name"/> : 
          <xsl:for-each select="picsom:choice">
            <input>
              <xsl:attribute name="type">radio</xsl:attribute>
              <xsl:attribute name="name">paramsel::<xsl:value-of 
	        select="$radioname"/></xsl:attribute>
              <xsl:attribute name="value"><xsl:value-of 
                select="@name"/></xsl:attribute>
              <xsl:if test="../@defval=@name">
                <xsl:attribute name="checked"/>
              </xsl:if>
              <xsl:attribute name="disabled">true</xsl:attribute>
            </input>
            <xsl:value-of select="@name"/> &#160;
          </xsl:for-each>
        </xsl:if>
      </td>
      <td><xsl:value-of select="@description"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:connectionlist">
    <h2> List of existing connections </h2>
    <table>
      <thead style="font-weight:bold">
        <tr>
          <td>type</td>
          <td>protocol</td>
          <td>state</td>
          <td>port</td>
          <td>wfd</td>
          <td>rfd</td>
          <td>identity</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:connection">
    <tr>
      <td><xsl:value-of select="picsom:type"/></td>
      <td><xsl:value-of select="picsom:protocol"/></td>
      <td><xsl:value-of select="picsom:state"/></td>
      <td><xsl:value-of select="picsom:port"/></td>
      <td><xsl:value-of select="picsom:wfd"/></td>
      <td><xsl:value-of select="picsom:rfd"/></td>
      <td><xsl:value-of select="picsom:identity"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:objects">
    <span id="objectlists" style=""> <!-- used to to display:none --> 
    <form>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="action">/action</xsl:attribute>
      <xsl:attribute name="enctype">multipart/form-data</xsl:attribute>
      <xsl:attribute name="name">f</xsl:attribute>
      <xsl:attribute name="id">f_id</xsl:attribute>
      <xsl:variable name="allowcontentselection" 
                    select="//picsom:database/picsom:allowcontentselection"/>
      <xsl:if test="$allowcontentselection='yes'">
	<xsl:call-template name="contentdisplaytemplate"/>
      </xsl:if>
      <h2> List of objects  </h2>
      <xsl:apply-templates select="picsom:questionobjectlist"/>
      <xsl:apply-templates select="picsom:plusobjectlist"/>
      <xsl:call-template name="guessedkeywords"/>
      <xsl:apply-templates select="picsom:minusobjectlist"/>
      <xsl:apply-templates select="picsom:zeroobjectlist"/>
      <xsl:apply-templates select="picsom:showobjectlist"/>
      <button type="button" name="selectall" onclick='select_all_checkboxes(this)'>Select all</button><xsl:text>
    </xsl:text>
      <button type="button" name="unselectall" onclick='unselect_all_checkboxes(this)'>Unselect all</button><xsl:text>
    </xsl:text>
    <button type="button" name="restart" onclick="window.location.href='/'">Restart</button>
    <xsl:variable name="resqlink">/database/<xsl:value-of select="//picsom:name"/></xsl:variable>
    <button type="button" name ="restartquery" onclick="window.location.href='{$resqlink}'">Restart query</button><xsl:text>
    </xsl:text>
      <span class="noannotation"><button type="button" onclick='toggle_annotations_on(this)'>Annotate</button></span>
      <span class="annotation" style="display:none"><input type="submit" name="annotations" value="Annotations done" onclick='toggle_annotations_off(this)'/></span>
      <br/><br/>
    <input type="submit" name="continue" value="Continue query"/><xsl:text>
  </xsl:text>
   </form>
  </span>

  <script type="text/javascript">
    showimages();
  </script>    
  </xsl:template>

  <xsl:template match="//picsom:plusobjectlist">
    <span><b>Relevant marked objects </b>(<span id="pluscount"></span>)
      <button type="button" onclick='toggle_table(this)'>hide</button>
    </span>
    <span>
    <table width="100%">
      <xsl:for-each select="picsom:objectinfo[position() mod $tncolumns = 1]">
        <tr>
          <xsl:apply-templates
            select=".|following-sibling::picsom:objectinfo[position() &lt; $tncolumns]"/>
        </tr>
      </xsl:for-each>
    </table>
  </span>
  <script type="text/javascript">
    set_nplusimages();
  </script>    
  <br/>
  </xsl:template>

  <xsl:template match="//picsom:minusobjectlist">
    <span><b>Non-relevant marked objects </b>(<span id="minuscount"></span>)
      <button type="button" onclick='toggle_table(this)'>show</button>
    </span>
    <span style="display:none">
    <table width="100%">
      <xsl:for-each select="picsom:objectinfo[position() mod $tncolumns = 1]">
        <tr>
          <xsl:apply-templates
            select=".|following-sibling::picsom:objectinfo[position() &lt; $tncolumns]"/>
        </tr>
      </xsl:for-each>
    </table>
  </span>
  <script type="text/javascript">
    set_nminusimages();
  </script>    
  <br/>
  </xsl:template>

  <xsl:template match="//picsom:zeroobjectlist">
    <span><b>Neutral marked objects </b>(<span id="zerocount"></span>)
      <button type="button" onclick='toggle_table(this)'>show</button>
    </span>
    <span style="display:none">
    <table width="100%">
      <xsl:for-each select="picsom:objectinfo[position() mod $tncolumns = 1]">
        <tr>
          <xsl:apply-templates
            select=".|following-sibling::picsom:objectinfo[position() &lt; $tncolumns]"/>
        </tr>
      </xsl:for-each>
    </table>
  </span>
  <script type="text/javascript">
    set_nzeroimages();
  </script>    
  <br/>
  </xsl:template>

  <xsl:template match="//picsom:showobjectlist">
    <span><b>Show objects </b>(<span id="showcount"></span>)
      <button type="button" onclick='toggle_table(this)'>show</button>
    </span>
    <span style="display:none">
    <table width="100%">
      <xsl:for-each select="picsom:objectinfo[position() mod $tncolumns = 1]">
        <tr>
          <xsl:apply-templates
            select=".|following-sibling::picsom:objectinfo[position() &lt; $tncolumns]"/>
        </tr>
      </xsl:for-each>
    </table>
  </span>
  <script type="text/javascript">
    set_nshowimages();
  </script>    
  <br/>
  </xsl:template>

  <xsl:template match="//picsom:questionobjectlist">
    <span><b>Query objects </b>(<span id="questioncount"></span>)
      <button type="button" onclick='toggle_table(this)'>hide</button>
    </span>
    <span>
    <table width="100%">
      <xsl:for-each select="picsom:objectinfo[position() mod $tncolumns = 1]">
        <tr>
          <xsl:apply-templates
            select=".|following-sibling::picsom:objectinfo[position() &lt; $tncolumns]"/>
        </tr>
      </xsl:for-each>
    </table>
  </span>
  <script type="text/javascript">
    set_nquestionimages();
  </script>    
  <br/>
  </xsl:template>

  <xsl:template match="//picsom:objectinfo">
    <!-- <h3> Image </h3> -->
    <!-- <td width="100%" height="100%" align="center" valign="center"> -->
    <xsl:text>
    </xsl:text>
       <td>
         <xsl:choose>
           <xsl:when test="picsom:selectable != 'true'"> 
             <xsl:call-template name="visibleimage"/>
           </xsl:when>
           <xsl:when test="$useradioselection=1"> 
             <xsl:call-template name="radioimage"/>
           </xsl:when>
           <xsl:otherwise>
             <xsl:call-template name="checkboximage"/>
           </xsl:otherwise>
         </xsl:choose>
      </td>
        <xsl:if test="(position() = last()) and (position() &lt; $tncolumns)">
          <xsl:call-template name="fillercells">
            <xsl:with-param name="cellCount" select="$tncolumns - position()"/>
          </xsl:call-template>
        </xsl:if>
  </xsl:template>

  <xsl:template name="fillercells">
    <xsl:param name="cellCount"/>
    <td>&#160;</td>
    <xsl:if test="$cellCount > 1">
      <xsl:call-template name="fillercells">
        <xsl:with-param name="cellCount" select="$cellCount - 1"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>
  
  <xsl:template name="checkboximage">
    <input>
      <xsl:attribute name="type">checkbox</xsl:attribute>
      <xsl:call-template name="imageinput"/>
    </input>
    <xsl:call-template name="visibleimage"/>
  </xsl:template>

  <xsl:template name="radioimage">
    <input>
      <xsl:attribute name="type">radio</xsl:attribute>
      <xsl:call-template name="imageinput"/>
    </input>
    <xsl:call-template name="visibleimage"/>
  </xsl:template>

  <xsl:template name="imageinput">
    <xsl:attribute name="name">obj-<xsl:value-of 
      select="name(..)"/></xsl:attribute>
    <xsl:attribute name="class"><xsl:value-of select="name(..)"/></xsl:attribute>
    <xsl:attribute name="value"><xsl:value-of select="picsom:label"/>,</xsl:attribute>
    <xsl:variable name="selected" select="picsom:aspectlist/picsom:aspect[@name='']/@value"/>
    <xsl:if test="$selected=1"><xsl:attribute name="checked"/></xsl:if>
    <xsl:attribute name="id">
      <xsl:value-of select="picsom:objectfileinfo/picsom:name"/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template name="clickableimage">
    <xsl:element name="a">
      <xsl:attribute name="href">/click/<xsl:value-of 
        select="picsom:database"/>/<xsl:value-of select="picsom:label"/>
      </xsl:attribute>
      <xsl:call-template name="visibleimageinner"/>
    </xsl:element>
  </xsl:template>

  <xsl:template name="visibleimage">
    <xsl:choose>
      <xsl:when test="../../picsom:singleobject">
	<b><xsl:value-of select="picsom:label"/>
	#<xsl:value-of select="picsom:index"/></b>
	<xsl:call-template name="visibleimageinner"/>
      </xsl:when>
      <xsl:otherwise>
	<xsl:call-template name="visibleimagewithlink"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="visibleimagewithlink">
    <xsl:variable name="queryordb">
      <xsl:choose>
        <xsl:when test="picsom:query"><xsl:value-of 
        select="picsom:query"/></xsl:when>
        <xsl:otherwise><xsl:value-of 
        select="picsom:database"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:element name="a">
      <xsl:attribute name="href">/objectinfo/<xsl:value-of 
      select="$queryordb"/>/<xsl:value-of select="picsom:label"/>
      </xsl:attribute>
      <xsl:call-template name="visibleimageinner"/>
    </xsl:element>
  </xsl:template>

  <xsl:template name="visibleimageinner">
    <xsl:element name="img">
      <xsl:variable name="selected" 
        select="picsom:aspectlist/picsom:aspect[@name='']/@value"/>
      <xsl:variable name="bordercolor">
        <xsl:choose>
          <xsl:when test="$selected=1">red</xsl:when>
          <xsl:otherwise>white</xsl:otherwise>
        </xsl:choose>
      </xsl:variable> 
      <xsl:attribute name="style">
        border:3px dashed <xsl:value-of select="$bordercolor"/>
      </xsl:attribute>
      <xsl:attribute name="src">
        /database/<xsl:value-of select="picsom:database"/>/<xsl:value-of
        select="picsom:label"/>?tn</xsl:attribute>
      <xsl:attribute name="id">obj<xsl:value-of select="picsom:label"/>
      </xsl:attribute>
      <xsl:variable name="score" select="picsom:score"/>
      <xsl:choose>
        <xsl:when test="$score > 0"> 
          <xsl:attribute name="title">Label: <xsl:value-of 
            select="picsom:label"/>, score: <xsl:value-of 
            select="picsom:score"/>
          </xsl:attribute>
        </xsl:when>
        <xsl:otherwise>
          <xsl:attribute name="title">Label: <xsl:value-of 
            select="picsom:label"/>
          </xsl:attribute>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:variable name="width">
	<xsl:choose>
	  <xsl:when test="picsom:width"><xsl:value-of select="picsom:width"/></xsl:when>
	  <xsl:otherwise><xsl:value-of select="picsom:frameinfo/picsom:width"/></xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <xsl:variable name="height">
	<xsl:choose>
	  <xsl:when test="picsom:height"><xsl:value-of select="picsom:height"/></xsl:when>
	  <xsl:otherwise><xsl:value-of select="picsom:frameinfo/picsom:height"/></xsl:otherwise>
	</xsl:choose>
      </xsl:variable>

      <xsl:choose>
        <xsl:when test="$tnheight*$width &gt; $tnwidth*$height">
	  <xsl:variable  name="seth" select="round($height div $width * $tnwidth)"/>
          <xsl:attribute name="width"><xsl:number  value="$tnwidth"/></xsl:attribute>
          <xsl:attribute name="height"><xsl:number value="$seth"/></xsl:attribute>
        </xsl:when>
        <xsl:otherwise>
	  <xsl:variable  name="setw" select="round($width div $height * $tnheight)"/>
          <xsl:attribute name="height"><xsl:number value="$tnheight"/></xsl:attribute>
          <xsl:attribute name="width"><xsl:number  value="$setw"/></xsl:attribute>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:element>
    <br/>
    <xsl:value-of select="picsom:label"/>
    <br/>
    <xsl:copy-of select="picsom:textsnippet"/>
    <xsl:call-template name="textindextexts"/>
    <br/>
    <xsl:call-template name="keywordlist"/>
    <xsl:if test="../../picsom:singleobject">
      <xsl:call-template name="html5video"/>
      <br/>
      <br/>
      <xsl:call-template name="timeline">
	<xsl:with-param name="spec">?segm=<xsl:value-of 
	select="$videosegm"/></xsl:with-param>
      </xsl:call-template>
      <br/>
      <xsl:call-template name="timeline">
	<xsl:with-param
	    name="spec">?detection=sbd::c_in12_z_fc6_a_ca3::20_0::,segm=<xsl:value-of 
	    select="$videosegm"/></xsl:with-param>
      </xsl:call-template>
      <br/>
      <xsl:call-template name="videosubobjects"/>
    </xsl:if>
  </xsl:template>

  <xsl:template name="html5video">
    <xsl:variable name="targettype">
      <xsl:value-of select="picsom:targettype"/>
    </xsl:variable>
    <xsl:if test="($targettype='segment+video') or ($targettype='file+video')">
      <br/>
      <table>
	<xsl:variable name="parent" select="picsom:parent"/>
	<xsl:variable name="videoprevious" select="picsom:videoprevious"/>
	<xsl:variable name="videonext" select="picsom:videonext"/>
	<xsl:variable name="videoss"><xsl:value-of 
	select="$videosegm"/></xsl:variable>
	<xsl:if test="picsom:videostart"><tr><td>start</td><td><xsl:value-of 
	select="picsom:videostart"/></td></tr></xsl:if>
	<xsl:if test="picsom:videoend"><tr><td>end</td><td><xsl:value-of 
	select="picsom:videoend"/></td></tr></xsl:if>
	<xsl:if test="picsom:videoduration"><tr><td>duration</td><td><xsl:value-of 
	select="picsom:videoduration"/></td></tr></xsl:if>
	<xsl:if test="$parent"><tr><td>parent</td><td>
	<a href="./{$parent}?segm={$videoss}"><xsl:value-of 
	select="$parent"/></a></td></tr></xsl:if>
	<xsl:if test="$videoprevious"><tr><td>previous</td><td>
	<a href="./{$videoprevious}"><xsl:value-of 
	select="$videoprevious"/></a></td></tr></xsl:if>
	<xsl:if test="$videonext"><tr><td>next</td><td>
	<a href="./{$videonext}"><xsl:value-of 
	select="$videonext"/></a></td></tr></xsl:if>
      </table>
      <br/>
      <xsl:variable name="mpsrc">/database/<xsl:value-of 
      select="picsom:database"/>/<xsl:value-of 
      select="picsom:label"/></xsl:variable>
      <xsl:variable name="ccsrc">/database/<xsl:value-of 
      select="picsom:database"/>/track-subtitles-en/<xsl:value-of 
      select="picsom:label"/>?segm=<xsl:value-of 
      select="$videosegm"/>:svmdbHASHlscom-demo;detections=f::c_in12_z_fc6_a_ca3::bb::;thr=0.9;showsegm=yes;showprob=yes</xsl:variable>
      <video width="500" height="300" controls="yes">
	<source src="{$mpsrc}" type="video/mp4"/>
	<track kind="subtitles" label="subtitleslabelEn"
	       src="{$ccsrc}" srclang="en"/>
      </video>
    </xsl:if>
  </xsl:template>

  <xsl:template name="timeline">
    <xsl:param name="spec"/>
    <xsl:variable name="targettype">
      <xsl:value-of select="picsom:targettype"/>
    </xsl:variable>
    <xsl:if test="($targettype='segment+video') or ($targettype='file+video')">
      <xsl:variable name="queryordb">
	<xsl:choose>
          <xsl:when test="picsom:query"><xsl:value-of 
          select="picsom:query"/></xsl:when>
          <xsl:otherwise><xsl:value-of 
          select="picsom:database"/></xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <xsl:variable name="tlimgsrc">/timeline/<xsl:value-of 
      select="picsom:database"/>/<xsl:value-of 
      select="picsom:label"/><xsl:value-of 
      select="$spec"/></xsl:variable>
      <form>
	<xsl:attribute name="action">/click/<xsl:value-of 
        select="$queryordb"/>/<xsl:value-of select="picsom:label"/>
	</xsl:attribute>
	<input type="image" width="500" height="100" src="{$tlimgsrc}"
	       name="timeline"/>
      </form>
    </xsl:if>
  </xsl:template>

  <xsl:template name="videosubobjects">
    <xsl:if test="picsom:subobjects/picsom:objectinfo">
    Subobjects:
    </xsl:if>
    <xsl:for-each select="picsom:subobjects/picsom:objectinfo">
      <br/>
      <xsl:variable name="label" select="picsom:label"/>
      <a href="./{$label}"><xsl:value-of select="$label"/></a>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="textindextexts">
    <span>
    <xsl:for-each select="picsom:textindextexts/picsom:text">
      <br/>
      <xsl:value-of select="@index"/>/<xsl:value-of select="@field"/>
      : <xsl:value-of select="."/>
    </xsl:for-each>
  </span>
  </xsl:template>

  <xsl:template name="keywordlist">
    <span xml:space="default" style="font-size:small">
       <xsl:for-each select="picsom:keywordlist/picsom:keyword">
	<xsl:if test="@value > 0">

	  <xsl:if test="@type='detection'"><span class="annotation" style="display:none"><br/></span></xsl:if>

	  <xsl:variable name="cname" select="@name"/>
	  <xsl:choose>
	    <xsl:when test="contains($cname, 'classdef::')">
	      <xsl:value-of select="substring-after($cname, 'classdef::')"/>
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:value-of select="$cname"/>
	    </xsl:otherwise>
	  </xsl:choose>
	  
	  <xsl:choose>
	    <xsl:when test="@type='gt'">, </xsl:when>
	    <xsl:otherwise> (<xsl:value-of select='format-number(round(@value*100) div 100, "##0.00")'/>)
	    <span xml:space="default" class="annotation" style="display:none;background-color:green;"><input>
	      <xsl:attribute name="type">radio</xsl:attribute>
	      <xsl:attribute name="name">classdef::<xsl:value-of select="../../picsom:label"/>,<xsl:value-of select="@name"/>,<xsl:value-of select="@type"/>
	      </xsl:attribute>
	      <xsl:attribute name="value">yes</xsl:attribute>
	      <xsl:attribute name="id"><xsl:value-of select="../../picsom:objectfileinfo/picsom:name"/>
	      </xsl:attribute>
	    </input></span>
	    <span xml:space="default" class="annotation" style="display:none;background-color:red;"><input>
	      <xsl:attribute name="type">radio</xsl:attribute>
	      <xsl:attribute name="name">classdef::<xsl:value-of select="../../picsom:label"/>,<xsl:value-of select="@name"/>,<xsl:value-of select="@type"/>
	      </xsl:attribute>
	      <xsl:attribute name="value">no</xsl:attribute>
	      <xsl:attribute name="id"><xsl:value-of select="../../picsom:objectfileinfo/picsom:name"/>
	      </xsl:attribute>
	    </input></span>
	    <span xml:space="default" class="annotation" style="display:none;background-color:grey;"><input>
	      <xsl:attribute name="type">radio</xsl:attribute>
	      <xsl:attribute name="name">classdef::<xsl:value-of select="../../picsom:label"/>,<xsl:value-of select="@name"/>,<xsl:value-of select="@type"/>
	      </xsl:attribute>
	      <xsl:attribute name="value">unset</xsl:attribute>
	      <xsl:attribute name="id"><xsl:value-of select="../../picsom:objectfileinfo/picsom:name"/>
	      </xsl:attribute>
	      </input></span>,
	    </xsl:otherwise>
	  </xsl:choose>
	  
	</xsl:if>
	
      </xsl:for-each>
    </span>

    <span xml:space="default" class="annotation" style="display:none;color:red;font-size:small">
      <br/>NOT: 
      <xsl:for-each select="picsom:keywordlist/picsom:keyword">
	<xsl:if test="@value&lt;0"><xsl:value-of select="@name"/>, </xsl:if>
      </xsl:for-each>
    </span>
  </xsl:template>

  <xsl:template name="guessedkeywords">
    <xsl:if test="//picsom:guessedkeywords/picsom:keyword">
      <b>Keywords</b>:
      <xsl:for-each select="//picsom:guessedkeywords/picsom:keyword">
        <xsl:value-of select="@name"/> (<xsl:value-of select="@value"/>) 
      </xsl:for-each>
      <br/><br/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="//picsom:guessedkeywords"/>

  <xsl:template match="picsom:contextstate">
    <h2> Context state variables </h2>
    <table border="1">
      <thead style="font-weight:bold">
        <tr>
          <td>name</td>
          <td>value</td>
          <td>time</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="picsom:contextstate/picsom:variable">
    <tr>
      <td><xsl:value-of select="@name"/></td>
      <td><xsl:value-of select="@value"/></td>
      <td><xsl:value-of select="@time"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="picsom:convtype"/>

  <xsl:template match="picsom:variables"/>

  <xsl:template match="picsom:parent_identity"/>

  <xsl:template match="picsom:xhtml"/>

  <xsl:template match="picsom:refresh"/>

  <xsl:template match="picsom:ajaxdata"/>

  <xsl:template match="picsom:childlist"/>

  <xsl:template match="//picsom:inrun"/>

  <xsl:template match="//picsom:versions">
    <h3> Versions </h3>
    <table>
      <thead style="font-weight:bold">
        <tr>
          <td>name</td>
          <td>version</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:piece">
    <tr>
      <td><xsl:value-of select="@name"/></td>
      <td><xsl:value-of select="@version"/></td>
    </tr>
  </xsl:template>

</xsl:stylesheet>

<!-- validate syntax with C-c C-v
Local Variables:
sgml-validate-command: "xmlstarlet val"
End:
-->
