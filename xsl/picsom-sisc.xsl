<?xml version="1.0" encoding="utf-8"?>

<!-- $Id: picsom-sisc.xsl,v 1.2 2009/09/15 09:13:39 jorma Exp $ -->

<!-- single image single click experiment --> 

<xsl:stylesheet 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:fo="http://www.w3.org/1999/XSL/Format"
  xmlns:picsom="http://www.cis.hut.fi/picsom/ns"
  version="1.0">

  <xsl:output method="html" indent="yes"/>

  <xsl:template match="/">
    <html>
      <head>
        <title>PicSOM content-based multimedia retrieval</title>
        <script src="/picsom.js" type="text/javascript"/>
      </head>
      <body>
        <!--
        <h1>PicSOM content-based multimedia retrieval</h1>
        -->
        <xsl:apply-templates/>
      </body>
    </html>
  </xsl:template>

  <xsl:template match="//picsom:link">
    <xsl:element name="a">
      <xsl:attribute name="href">
        /<xsl:value-of select="@href"/></xsl:attribute>
      <xsl:value-of select="@href"/>
    </xsl:element>
    Description of <xsl:value-of select="@href"/>
    <xsl:element name="br"/>
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

  <xsl:template match="//picsom:threadlist">
    <h2> List of existing threads </h2>
    <table>
      <xsl:apply-templates/>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:thread">
    <tr>
      <td><xsl:value-of select="picsom:name"/></td>
      <td><xsl:value-of select="picsom:pthread"/></td>
      <td><xsl:value-of select="picsom:phase"/></td>
      <td><xsl:value-of select="picsom:type"/></td>
      <td><xsl:value-of select="picsom:text"/></td>
      <td><xsl:value-of select="picsom:state"/></td>
      <td><xsl:value-of select="picsom:slave_state"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:databaselist">
    <h2> List of available databases </h2>
    <table>
      <xsl:apply-templates/>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:database">
    <!--
    <tr>
      <td>
        <xsl:element name="a">
          <xsl:attribute name="href">
            /database/<xsl:value-of select="picsom:name"/>
          </xsl:attribute>
          <xsl:value-of select="picsom:name"/>
        </xsl:element>
      </td>
      <td><xsl:value-of select="picsom:longname"/></td>
      <td>
        <span><xsl:value-of select="picsom:shorttext"/></span>
        <span style="display:none"><xsl:value-of select="picsom:longtext"/></span>
        <form>
          <button type="button" onclick='toggle_short_long(this)'> MORE </button>
        </form>
      </td>
      <td><xsl:value-of select="picsom:size"/></td>
      <td><xsl:value-of select="picsom:objecttypes"/></td>
    </tr>
    -->
  </xsl:template>

  <xsl:template match="//picsom:featurelist">
    <!--
    <h2> List of available features </h2>
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
    -->
  </xsl:template>

  <xsl:template match="//picsom:feature">
    <tr>
      <td>
        <input>
          <xsl:attribute name="type">checkbox</xsl:attribute>
          <xsl:attribute name="name">featsel</xsl:attribute>
          <xsl:attribute name="value"><xsl:value-of select="picsom:name"/></xsl:attribute>
          <xsl:attribute name="checked">checked</xsl:attribute>
          <xsl:attribute name="id"><xsl:value-of select="picsom:name"/></xsl:attribute>
        </input>
      </td>
      <td><xsl:value-of select="picsom:longname"/></td>
      <td><xsl:value-of select="picsom:shorttext"/></td>
      <td><xsl:value-of select="picsom:size"/></td>
      <td><xsl:value-of select="picsom:objecttype"/></td>
      <td><xsl:value-of select="picsom:counts"/></td>
      <td><xsl:value-of select="picsom:type"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:algorithmlist">
    <!--
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
    -->
  </xsl:template>

  <xsl:template match="//picsom:algorithm">
    <!--
    <tr>
      <td>
        <input>
          <xsl:attribute name="type">checkbox</xsl:attribute>
          <xsl:attribute name="name">algsel</xsl:attribute>
          <xsl:attribute name="value"><xsl:value-of select="picsom:name"/></xsl:attribute>
          <xsl:attribute name="checked">checked</xsl:attribute>
          <xsl:attribute name="id"><xsl:value-of select="picsom:name"/></xsl:attribute>
        </input>
      </td>
      <td><xsl:value-of select="picsom:name"/></td>
      <td><xsl:value-of select="picsom:label"/></td>
      <td><xsl:value-of select="picsom:description"/></td>
      <td><xsl:value-of select="picsom:order"/></td>
      <td><xsl:value-of select="picsom:default"/></td>
    </tr>
    -->
  </xsl:template>

  <xsl:template match="//picsom:objects">
    <!-- <h2> List of images  </h2> -->
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="//picsom:questionobjectlist">
    <!-- <h3> Query images </h3> -->
    <table width="100%" height="530">
      <xsl:apply-templates/>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:objectinfo">
    <!-- <h3> Image </h3> -->
    <tr>
      <td width="100%" height="100%" align="center" valign="center">
        <xsl:element name="a">
          <xsl:attribute name="href">/click/<xsl:value-of select="picsom:database"/>/<xsl:value-of select="picsom:label"/>
          </xsl:attribute>
          <xsl:element name="img">
            <xsl:attribute name="src">
              /database/<xsl:value-of select="picsom:database"/>/<xsl:value-of select="picsom:label"/>
            </xsl:attribute>
            <xsl:attribute name="ismap"/>
          </xsl:element>
        </xsl:element>
      </td>
    </tr>
  </xsl:template>

  <xsl:template match="picsom:contentitemlist"/>

  <xsl:template match="picsom:contentitem"/>

  <xsl:template match="picsom:convtype"/>

  <xsl:template match="picsom:variables"/>

  <xsl:template match="picsom:xhtml"/>

  <xsl:template match="//picsom:inrun"/>

</xsl:stylesheet>
